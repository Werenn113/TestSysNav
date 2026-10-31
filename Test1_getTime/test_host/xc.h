///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		xc.h
///
/// @brief		MOCK POUR PC du <xc.h> du compilateur XC16 (cible reelle : dsPIC33EP512MU814).
///
/// @date		24/10/2026
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// CE FICHIER N'EST PAS DU CODE PRODUIT : c'est un simulateur minimaliste du dsPIC33, destine
/// a compiler et tester VOTRE Time.c sur un PC (gcc, clang, onlinegdb...). Il simule :
///
///   - le Timer2/3 en mode 32 bits (TMR3:TMR2), horloge Fcy = 30 MHz, wrap a PR3:PR2 + 1 ;
///   - la lecture coherente TMR2 puis TMR3HLD (lecture de TMR2 = fige le poids fort) ;
///   - le drapeau T3IF, l'autorisation T3IE, la priorite T3IP, le registre SR (IPL) ;
///   - la latence de l'interruption Timer3 (configurable, meme aleatoire) ;
///   - le "chaos" : chaque acces a un registre SFR consomme 0 a 7 cycles (ou une valeur
///     fixe pour les tests deterministes) -> le timer tourne PENDANT vos lectures,
///     exactement comme sur le vrai matériel.
///
/// UTILISATION : ne pas modifier. Seul TEST_Time.c definit MOCK_XC_DEFINE avant d'inclure
/// ce fichier ; les definitions du simulateur vivent alors dans TEST_Time.c, et Time.c
/// (compile sans -DMOCK_XC_DEFINE) n'en voit que les declarations.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _XC_MOCK_H
#define _XC_MOCK_H

#include <stdint.h>		/* types fixes du mock (avant la neutralisation des attributs) */

/* Les attributs specifiques XC16 (__interrupt__, no_auto_psv, packed...) n'existent pas
 * sur un compilateur PC : on les neutralise pour que le code cible compile tel quel. */
#define __attribute__(...)

/* ---------------------------------- registres mocketes --------------------------------------- */

/// bits de T2CON utiles au test (TON, mode 32 bits, prescaler, source d'horloge)
typedef struct {
	volatile unsigned int TON	:1;	///< demarrage du timer
	volatile unsigned int TSIDL	:1;	///< arret en mode veille (non simule)
	volatile unsigned int T32	:1;	///< mode 32 bits : TMR3:TMR2 concatenees
	volatile unsigned int TCS	:1;	///< horloge interne (0) / externe (1)
	volatile unsigned int TGATE	:1;	///< mode gatel (non simule)
	volatile unsigned int TCKPS	:3;	///< prescaler : 0 = 1:1
} T2CONbits_t;

/// bits utiles de IFS0 (drapeau d'interruption Timer3)
typedef struct {
	volatile unsigned int T3IF	:1;	///< debordement survenu, non encore acquitte
} IFS0bits_t;

/// bits utiles de IEC0 (autorisation d'interruption Timer3)
typedef struct {
	volatile unsigned int T3IE	:1;	///< interruption autorisee
} IEC0bits_t;

/// bits utiles de IPC2 (priorite d'interruption Timer3 : 0 = DESACTIVEE sur dsPIC33 !)
typedef struct {
	volatile unsigned int T3IP	:3;	///< priorite 0..7
} IPC2bits_t;

extern T2CONbits_t T2CONbits;		///< registre de configuration du timer (acces direct)
extern IEC0bits_t	IEC0bits;		///< autorisation d'interruption (acces direct)
extern IPC2bits_t	IPC2bits;		///< priorite d'interruption (acces direct)
extern uint16_t		PR2;			///< periode, poids faibles
extern uint16_t		PR3;			///< periode, poids forts
extern uint16_t		SR;			///< registre d'etat CPU (IPL = bits 5..7)

/* IFS0bits passe par un accesseur : chaque lecture simule le temps d'acces au bus
 * (le temps avance AVANT que la valeur ne soit lue, comme sur le vrai materiel). */
IFS0bits_t *mockIFS0bitsPtr(void);
#define IFS0bits	(*mockIFS0bitsPtr())

/* Lecture coherente du compteur 32 bits : lire TMR2 fige le poids fort dans TMR3HLD
 * (semantique reelle du mode 32 bits du dsPIC33). L'ecriture de TMR2/TMR3HLD par le
 * code candidat est ignoree par le mock (Time_Init est cense repartir de zero ; le
 * harnais appelle sim_start() juste apres pour remettre la timeline a zero). */
uint16_t *mockTMR2Ptr(void);
uint16_t *mockTMR3HLDPtr(void);
#define TMR2		(*mockTMR2Ptr())
#define TMR3HLD	(*mockTMR3HLDPtr())

/* Interruption Timer3 : implementee par le code candidat (Time.c). */
extern void _T3Interrupt(void);

/* ------------------------------------ etat du simulateur ------------------------------------- */

/// Etat interne du simulateur (champs consultables/configurables par TEST_Time.c)
typedef struct {
	uint64_t cycles;		///< cycles Fcy ecoules depuis sim_start()
	uint64_t nextWrap;		///< cycle du prochain debordement
	uint64_t isrDue;		///< cycle ou l'IT Timer3 doit s'executer (UINT64_MAX = rien)
	uint64_t lastTmr2Read;		///< cycle de la derniere lecture de TMR2 (reference des tests)
	uint64_t isrLatency;		///< latence artificielle de l'IT (en cycles) si !randomLatency
	uint64_t seed;			///< graine du generateur du "chaos" (LCG 64 bits deterministe)
	int64_t  chaosFixed;		///< >= 0 : chaque acces SFR consomme exactement ce nombre de cycles
	uint32_t chaosMax;		///< sinon, tirage 0..chaosMax cycles par acces SFR
	uint32_t wraps;			///< nombre de debordements survenus
	uint32_t isrs;			///< nombre d'executions de _T3Interrupt()
	uint32_t deferred;		///< debordements dont l'IT n'a pas pu s'executer a l'heure (IPL)
	uint8_t  inIsr;			///< _T3Interrupt() en cours (interdit la recursion)
	uint8_t  randomLatency;		///< 1 : latence de l'IT tiree aleatoirement 0..999999 a chaque wrap
	uint16_t tmr2Val;		///< derniere valeur "lue" de TMR2
	uint16_t tmr3Latch;		///< poids fort fige par la derniere lecture de TMR2
} SimState;

extern SimState sim;

/* ------------------------------------ API du simulateur -------------------------------------- */

void	 sim_start(void);		///< remet la timeline a zero (a appeler APRES Time_Init())
void	 sim_advance(uint64_t cycles);	///< fait tourner le temps du simulateur
void	 sim_set_ticks(uint64_t ticks);	///< positionne le compteur dans la periode courante
uint64_t sim_period(void);		///< periode courante du timer = PR3:PR2 + 1 cycles

#endif /* !_XC_MOCK_H */

/* ================================ DEFINITIONS (TEST_Time.c uniquement) ====================== */
#ifdef MOCK_XC_DEFINE

#include <stdio.h>

T2CONbits_t	T2CONbits;		/* registres "simples" : acces direct, pas de temps simule */
IEC0bits_t	IEC0bits;
IPC2bits_t	IPC2bits;
uint16_t	PR2;
uint16_t	PR3;
uint16_t	SR;

static IFS0bits_t	mock_ifs0;	/* drapeau T3IF (via accesseur pour le code candidat) */
SimState		sim;

/// Valeur du compteur 32 bits a l'instant present
static uint64_t sim_currentTicks(void) {
	return sim.cycles - (sim.nextWrap - sim_period());
}

/// Niveau de priorite CPU courant (IPL, bits 5..7 de SR)
static uint8_t sim_ipl(void) {
	return (uint8_t)((SR >> 5) & 7);
}

/// Tirage pseudo-aleatoire deterministe (LCG 64 bits) du temps d'acces aux SFR
static uint32_t sim_chaos(void) {
	if (sim.chaosFixed >= 0) {
		return (uint32_t)sim.chaosFixed;
	}
	sim.seed = sim.seed * 6364136223846793005ULL + 1442695040888963407ULL;
	return (uint32_t)((sim.seed >> 33) % (sim.chaosMax + 1));
}

/// Execute l'IT Timer3 si elle est due et autorisee (priorite CPU < T3IP)
/// Renvoie 1 si l'IT a ete executee, 0 sinon.
static uint8_t sim_tryDeliver(void) {
	if (mock_ifs0.T3IF && IEC0bits.T3IE && IPC2bits.T3IP > 0
	    && sim.isrDue <= sim.cycles
	    && sim_ipl() < IPC2bits.T3IP
	    && !sim.inIsr) {
		sim.inIsr = 1;
		sim.isrs++;
		_T3Interrupt();		/* code du candidat : acquitte T3IF, incremente les secondes */
		sim.inIsr = 0;
		return 1;
	}
	return 0;
}

void sim_advance(uint64_t cycles) {
	uint64_t target;

	if (!T2CONbits.TON) {
		return;			/* timer arrete : le temps ne compte plus */
	}

	(void)sim_tryDeliver();		/* livraison d'une IT restee en attente (demasquee entre-temps) */

	target = sim.cycles + cycles;
	while (sim.nextWrap <= target) {
		uint64_t wrap = sim.nextWrap;
		uint64_t latency;

		sim.cycles = wrap;
		mock_ifs0.T3IF = 1;	/* debordement materiel : drapeau leve */
		sim.wraps++;

		/* latence de l'interruption (mode aleatoire pour le test T4) */
		latency = sim.randomLatency ? (uint64_t)(sim_chaos() % 1000000u)
		                            : sim.isrLatency;
		sim.isrDue = wrap + latency;
		sim.nextWrap = wrap + sim_period();

		if (!sim_tryDeliver() && sim.isrDue <= sim.cycles) {
			sim.deferred++;	/* IT due mais masquee (IPL trop haut) : elle attend */
		}
	}
	sim.cycles = target;
}

void sim_start(void) {
	sim.cycles = 0;
	sim.nextWrap = sim_period();
	sim.isrDue = UINT64_MAX;
	sim.lastTmr2Read = 0;
	sim.wraps = 0;
	sim.isrs = 0;
	sim.deferred = 0;
	sim.inIsr = 0;
	sim.tmr2Val = 0;
	sim.tmr3Latch = 0;
}

void sim_set_ticks(uint64_t ticks) {
	uint64_t period = sim_period();
	uint64_t current = sim_currentTicks();
	uint64_t delta = ((ticks % period) + period - current) % period;
	sim_advance(delta);
}

uint64_t sim_period(void) {
	return (((uint64_t)PR3 << 16) | PR2) + 1;
}

/* --------------------------- accesseurs "chaotiques" du candidat ----------------------------- */

uint16_t *mockTMR2Ptr(void) {
	uint64_t ticks32;

	if (!sim.inIsr) {
		sim_advance(sim_chaos());	/* le temps passe pendant l'acces... */
	}
	ticks32 = sim_currentTicks();		/* ...puis la valeur est lue */
	sim.lastTmr2Read = sim.cycles;		/* reference des tests : instant de cette lecture */
	sim.tmr3Latch = (uint16_t)(ticks32 >> 16);	/* fige le poids fort (mode 32 bits) */
	sim.tmr2Val = (uint16_t)ticks32;
	return &sim.tmr2Val;
}

uint16_t *mockTMR3HLDPtr(void) {
	if (!sim.inIsr) {
		sim_advance(sim_chaos());
	}
	return &sim.tmr3Latch;		/* poids fort fige par la derniere lecture de TMR2 */
}

IFS0bits_t *mockIFS0bitsPtr(void) {
	if (!sim.inIsr) {
		sim_advance(sim_chaos());
	}
	return &mock_ifs0;
}

#endif /* MOCK_XC_DEFINE */
