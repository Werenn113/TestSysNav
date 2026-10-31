///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		TEST_Time.c
///
/// @brief		Harnais de test PC pour Time.c — Test SYSNAV code embarque #1 (getTime)
///
/// @date		24/10/2026
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// MODE D'EMPLOI
/// -------------
/// 1. Recuperer un compilateur C :
///    - en ligne (rien a installer) : https://www.onlinegdb.com  -> langue "C"
///    - en local : gcc (Linux/WSL/MSYS2), clang, MinGW... voir le plan de travail.
///
/// 2. Organiser les fichiers (depuis le dossier racine du test) :
///
///        LibC_reduite/Os/Common/Utils/GenericTypeDefs.h
///        Os/Pic/Time/Time.c        <- VOTRE implementation
///        Os/Pic/Time/Time.h
///        test_host/xc.h            <- le mock dsPIC33 (ne pas modifier)
///        TEST_Time.c               <- ce fichier
///
/// 3. Compiler et lancer :
///
///        gcc -std=c99 -Wall -Wextra -I test_host -I LibC_reduite -o test_time TEST_Time.c Os/Pic/Time/Time.c
///        ./test_time
///
///    (sur onlinegdb : coller Time.c dans un onglet, xc.h et TEST_Time.c dans les
///     autres, choisir "C", puis dans les options du projet ajouter les includes
///     -I. et remplacer l'execution simple par le contenu de Time.c.)
///
/// CE QUE LES TESTS VERIFIENT
/// --------------------------
/// T1  Configuration materielle (PR, mode 32 bits, priorite T3IP, autorisation T3IE)
/// T2  Exactitude de base : 2000 lectures a des instants aleatoires (erreur < 3 us)
/// T3a SONDE : le compteur boucle PENDANT la lecture -> piege a 1 000 000 us
/// T3b SONDE : le compteur boucle ENTRE la lecture du compteur et celle du drapeau
///     (le piege des pieges : le timer ne s'arrete pas pendant votre section critique)
/// T4  Soupe aleatoire : 2000 lectures avec latence d'interruption aleatoire 0..1 s
/// T5  Monotonie : le temps retourne ne diminue jamais
/// T6  Dix heures d'utilisation en avance rapide : 36 000 wraps, 36 000 IT, temps exact
///
/// Le simulateur rend les erreurs DETERMINISTES : les memes tests echouent toujours de
/// la meme facon tant que le code n'est pas corrige, et reussissent ensuite a coup sur.
///////////////////////////////////////////////////////////////////////////////////////////////////

#define MOCK_XC_DEFINE
#include <stdio.h>
#include "xc.h"
#include "Time.h"

/* ---------------------------------- utilitaires du harnais ---------------------------------- */

static uint64_t hseed = 0;	///< graine du tirage du harnais (LCG 64 bits, deterministe)

/// Tirage pseudo-aleatoire 0..n-1 (independant du "chaos" du simulateur)
static uint32_t hrnd(uint32_t n) {
	hseed = hseed * 6364136223846793005ULL + 1442695040888963407ULL;
	return (uint32_t)((hseed >> 33) % n);
}

/// Avance le temps par paquets (pour laisser l'IT Timer3 se declencher a l'heure)
static void advanceChunks(uint64_t total, uint64_t chunk) {
	uint64_t left = total;
	while (left > 0) {
		uint64_t step = (left < chunk) ? left : chunk;
		sim_advance(step);
		left -= step;
	}
}

/// Ecart [us] entre la valeur retournee par getTime() et la verite du simulateur
/// (reference : le cycle de la derniere lecture de TMR2 faite pendant l'appel)
static double errUs(Time t) {
	return (double)t - (double)sim.lastTmr2Read / 30.0;
}

/* --------------------------------------------- T1 -------------------------------------------- */

static int testT1(void) {
	int fails = 0;

	printf("[T1] Configuration materielle\n");
	Time_Init();
	sim_start();
	sim.seed = 1;
	sim.chaosFixed = -1;
	sim.chaosMax = 7;
	sim.randomLatency = 0;
	sim.isrLatency = 0;

	if ((PR2 != 50047) || (PR3 != 457)) {
		printf("     ECHEC : periode PR3:PR2 = 0x%04X%04X = %lu, attendu 0x01C9C37F = 29 999 999\n",
		       PR3, PR2, (unsigned long)(((uint32_t)PR3 << 16) | PR2));
		printf("     (rappel : le compteur va de 0 a PR inclus, soit PR+1 cycles par periode)\n");
		fails++;
	}
	if (!T2CONbits.TON || !T2CONbits.T32 || (T2CONbits.TCKPS != 0) || (T2CONbits.TCS != 0)) {
		printf("     ECHEC : T2CON -> TON=%u T32=%u TCKPS=%u TCS=%u (attendu 1 1 0 0)\n",
		       T2CONbits.TON, T2CONbits.T32, T2CONbits.TCKPS, T2CONbits.TCS);
		fails++;
	}
	if (IPC2bits.T3IP < 1) {
		printf("     ECHEC : T3IP = %u. Sur dsPIC33, une priorite 0 laisse l'interruption\n",
		       IPC2bits.T3IP);
		printf("     DESACTIVEE : le compteur de secondes ne s'incrementerait jamais !\n");
		fails++;
	}
	if (!IEC0bits.T3IE) {
		printf("     ECHEC : T3IE = 0, l'interruption Timer3 n'est pas autorisee\n");
		fails++;
	}
	if (IFS0bits.T3IF != 0) {
		printf("     ECHEC : T3IF n'est pas nettoye apres Time_Init()\n");
		fails++;
	}

	if (fails == 0) {
		printf("     OK : PR3:PR2 = 29 999 999, mode 32 bits, T3IP = %u, T3IE = 1\n",
		       IPC2bits.T3IP);
	}
	return fails;
}

/* --------------------------------------------- T2 -------------------------------------------- */

static int testT2(void) {
	int fails = 0;
	uint32_t i;
	double maxErr = 0.0;
	uint32_t monoBad = 0;
	Time prev = 0;
	Time first = 1;		/* sentinelle "pas encore d'appel" */

	printf("[T2] Exactitude de base : 2000 lectures aleatoires, IT immediate\n");
	hseed = 1000;
	sim.chaosFixed = -1;
	sim.chaosMax = 7;
	sim.randomLatency = 0;
	sim.isrLatency = 0;

	for (i = 0; i < 2000; i++) {
		Time t;
		double e;

		advanceChunks(hrnd(30000000u), 100000);
		t = getTime();
		e = errUs(t);
		if (e < 0) {
			e = -e;
		}
		if (e > maxErr) {
			maxErr = e;
		}
		if (first) {
			first = 0;
		} else if (t < prev) {
			monoBad++;
		}
		prev = t;
	}

	if (maxErr > 3.0) {
		printf("     ECHEC : erreur max %.2f us (> 3 us)\n", maxErr);
		printf("     - erreur ~multiple de 256 us ou valeur absurde ? -> le cast (Time) tronque\n");
		printf("       surement a 8 bits : le typedef uint8 d'origine est un PIEGE du sujet.\n");
		printf("     - erreur qui grossit avec le temps ? -> periode PR a revoir (T1).\n");
		fails++;
	}
	if (monoBad != 0) {
		printf("     ECHEC : %u lectures non monotones\n", monoBad);
		fails++;
	}
	if (fails == 0) {
		printf("     OK : erreur max %.2f us, monotonie respectee\n", maxErr);
	}
	return fails;
}

/* --------------------------------------- T3a / T3b ------------------------------------------- */

/// Sonde deterministe : place le compteur a "offset" cycles du debordement, avec un
/// temps d'acces SFR fixe de 200 cycles par lecture, et une IT retardee de 400 ms.
/// T3a (offset 200) : le debordement tombe PENDANT la lecture du compteur.
/// T3b (offset 400) : le debordement tombe ENTRE la lecture du compteur et celle du
///                    drapeau T3IF (le compteur capturé est pre-debordement !).
static int testProbe(uint32_t offset, const char *name, const char *lesson) {
	int fails = 0;
	uint32_t k;
	double err = 0.0;

	printf("[T3%s] %s\n", name, lesson);
	sim.chaosFixed = 200;
	sim.isrLatency = 400000;
	sim.randomLatency = 0;

	for (k = 0; k < 3; k++) {
		Time t;

		sim_set_ticks(sim_period() - offset);
		t = getTime();
		err = errUs(t);

		/* repartir proprement : rendre l'IT en attente, avance rapide */
		sim.chaosFixed = -1;
		advanceChunks(50000000, 100000);
		sim.chaosFixed = 200;
	}

	if ((err < -30.0) || (err > 30.0)) {
		printf("     ECHEC : erreur %.2f us au moment du debordement (tolerance 30 us)\n", err);
		if (err < -500000.0) {
			printf("     -> votre getTime retourne UNE SECONDE DE MOINS : le debordement n'est\n");
			printf("        pas compte. Le drapeau T3IF reste leve tant que l'IT n'a pas ete\n");
			printf("        executee : il FAUT le lire et corriger les secondes vous-meme.\n");
		} else {
			printf("     -> votre getTime retourne UNE SECONDE DE TROP : le compteur capture\n");
			printf("        etait pre-debordement. Meme sous section critique, le timer\n");
			printf("        MATERIEL continue de tourner ! Si le drapeau est leve, relisez le\n");
			printf("        compteur : s'il est redescendu, le debordement date de votre propre\n");
			printf("        capture -> il ne faut PAS compter cette seconde.\n");
		}
		fails++;
	} else {
		printf("     OK : erreur %.2f us meme avec l'IT retardee de 400 ms\n", err);
	}
	return fails;
}

/* --------------------------------------------- T4 -------------------------------------------- */

static int testT4(void) {
	int fails = 0;
	uint32_t i;
	double maxErr = 0.0;
	uint32_t monoBad = 0;
	Time prev;

	printf("[T4] Soupe aleatoire : 2000 lectures, latence d'IT aleatoire 0..1 s\n");
	hseed = 2000;
	sim.chaosFixed = -1;
	sim.chaosMax = 7;
	sim.randomLatency = 1;
	sim.isrLatency = 0;

	prev = getTime();	/* mise en repere de la monotonie */

	for (i = 0; i < 2000; i++) {
		Time t;
		double e;

		if ((i % 5u) == 0) {
			/* 1 appel sur 5 : placer le compteur juste avant le debordement */
			sim_set_ticks(sim_period() - 1 - hrnd(1000));
			advanceChunks(hrnd(50000u), 50000);
		} else {
			advanceChunks(hrnd(5000000u), 50000);
		}
		t = getTime();
		e = errUs(t);
		if (e < 0) {
			e = -e;
		}
		if (e > maxErr) {
			maxErr = e;
		}
		if (t < prev) {
			monoBad++;
		}
		prev = t;
	}

	if (maxErr > 100.0) {
		printf("     ECHEC : erreur max %.2f us (> 100 us, la spec du sujet)\n", maxErr);
		printf("     -> si l'erreur vaut ~1 000 000 us : debordement mal gere (voir T3a/T3b)\n");
		printf("     -> si l'erreur est petite mais > 100 us : lecture non coherente\n");
		fails++;
	}
	if (monoBad != 0) {
		printf("     ECHEC : %u lectures non monotones\n", monoBad);
		fails++;
	}
	if (fails == 0) {
		printf("     OK : erreur max %.2f us quelle que soit la latence de l'IT\n", maxErr);
		printf("     (%u debordements, dont %u executes en retard par le simulateur)\n",
		       sim.wraps, sim.deferred);
	}
	return fails;
}

/* --------------------------------------------- T5 -------------------------------------------- */

static int testT5(void) {
	int fails = 0;
	uint32_t i;
	uint32_t monoBad = 0;
	Time prev;

	printf("[T5] Monotonie en rafale (1000 appels serres)\n");
	hseed = 3000;
	sim.randomLatency = 0;
	sim.isrLatency = 0;
	sim.chaosFixed = -1;
	sim.chaosMax = 7;

	prev = getTime();
	for (i = 0; i < 1000; i++) {
		Time t;

		sim_advance(hrnd(200u));
		t = getTime();
		if (t < prev) {
			monoBad++;
		}
		prev = t;
	}

	if (monoBad != 0) {
		printf("     ECHEC : %u retours en arriere : le temps doit etre monotone\n", monoBad);
		printf("     -> typiquement un debordement compte deux fois ou pas du tout\n");
		fails++;
	} else {
		printf("     OK : jamais de retour en arriere\n");
	}
	return fails;
}

/* --------------------------------------------- T6 -------------------------------------------- */

static int testT6(void) {
	int fails = 0;
	uint32_t i;
	Time t;
	double e;

	printf("[T6] Dix heures d'utilisation en avance rapide\n");
	Time_Init();
	sim_start();
	sim.chaosFixed = 0;
	sim.randomLatency = 0;
	sim.isrLatency = 0;
	sim.seed = 7;

	for (i = 0; i < 1080; i++) {
		sim_advance(1000000000ULL);	/* 1080 x 1e9 cycles = 1,08e12 cycles = 36 000 s */
	}

	t = getTime();
	e = errUs(t);

	if ((sim.wraps != 36000) || (sim.isrs != 36000)) {
		printf("     ECHEC : %u debordements, %u executions de l'IT (attendu 36 000 / 36 000)\n",
		       sim.wraps, sim.isrs);
		printf("     -> si l'IT est executee trop souvent : T3IF n'est pas acquitte par\n");
		printf("        votre _T3Interrupt, ou la priorite T3IP est trop haute/negative\n");
		fails++;
	}
	if ((t != (Time)36000000000ULL) || (e > 3.0) || (e < -3.0)) {
		printf("     ECHEC : t = %llu us apres 10 h (attendu 36 000 000 000 us)\n",
		       (unsigned long long)t);
		fails++;
	}
	if (fails == 0) {
		printf("     OK : 36 000 debordements, 36 000 IT, t = %llu us exactement\n",
		       (unsigned long long)t);
	}
	return fails;
}

/* -------------------------------------------- main ------------------------------------------- */

int main(void) {
	int fails = 0;

	printf("=====================================================================\n");
	printf("  TEST_Time.c - harnais PC pour Time.c (test SYSNAV code embarque #1)\n");
	printf("  Simulateur : Timer2/3 32 bits @30 MHz, periode 1 s, dsPIC33 mockete\n");
	printf("=====================================================================\n\n");

	fails += testT1();
	fails += testT2();
	fails += testProbe(200, "a", "Sonde : debordement PENDANT la lecture du compteur");
	fails += testProbe(400, "b", "Sonde : debordement APRES la lecture, AVANT le drapeau");
	fails += testT4();
	fails += testT5();
	fails += testT6();

	printf("\n---------------------------------------------------------------------\n");
	if (fails == 0) {
		printf("  RESULTAT : 7/7 tests OK. Votre getTime est robuste au wrap, a la\n");
		printf("  latence d'interruption et a 10 h d'utilisation. Defendez-la en\n");
		printf("  entretien : PR = 29 999 999, section critique IPL=7, correction T3IF,\n");
		printf("  relecture du compteur, Time 64 bits.\n");
	} else {
		printf("  RESULTAT : %d test(s) en echec. Corrigez, relancez : chaque echec\n", fails);
		printf("  est deterministe, il disparaitra avec la bonne correction.\n");
	}
	printf("---------------------------------------------------------------------\n");

	return (fails == 0) ? 0 : 1;
}
