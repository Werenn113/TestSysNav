///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		Time.c
///  
/// @brief		Gestion du temps
///  
/// @date		15/04/2014
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Time.h"
#include <xc.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Frequence des instructions (Fcy) :
/// quartz 16 MHz, PLL reglee dans systemPowerUp() : M = PLLFBD+2 = 15, N1 = PLLPRE+2 = 2,
/// N2 = 2x(PLLPOST+1) = 2  ->  Fosc = 16 x 15 / (2 x 2) = 60 MHz  ->  Fcy = Fosc/2 = 30 MHz
#define TIME_FCY_HZ			(uint32)30000000

/// Periode du timer 32 bits : 1 seconde = 30 000 000 cycles.
/// Le registre PR3:PR2 est charge avec (periode - 1) car le compteur compte de 0 a PR inclus,
/// soit (PR+1) cycles par periode -> PR = 29 999 999 pour une seconde exacte.
#define TIME_PR_MAX			(uint32)(TIME_FCY_HZ - 1)

/// Nombre de ticks Fcy par microseconde (30 MHz -> 30 ticks/us)
#define TIME_TICKS_PER_US	(uint32)30

/// Priorite de l'interruption Timer3.
/// Compteur lent et non critique : getTime() gere le cas ou cette interruption est
/// retardee (via le drapeau T3IF), aucune contrainte de latence n'est imposee.
/// ATTENTION : priorite 0 = interruption desactivee sur dsPIC33.
#define TIME_T3_PRIORITY	(uint8)5

/// Temps systeme en secondes depuis la mise sous tension.
/// volatile : ecrite par l'interruption Timer3, lue par le code projet / peripheriques.
static volatile uint32 timeSeconds = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////
// TIME INIT
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Initialise le service temps.
/// Timer2 + Timer3 concatenees en mode 32 bits (TMR3:TMR2), horloge Fcy, prescaler 1:1.
/// Le debordement (match de periode 32 bits, signale par Timer3) a lieu chaque seconde
/// exactement et genere une interruption qui incremente timeSeconds.
/// Le temps t = 0 correspond a l'appel de Time_Init() (cf. note explicative).
void Time_Init(void) {
	uint16 prHigh;
	uint16 prLow;

	// configuration du timer 32 bits
	T2CONbits.TON = 0;						// arret pendant la configuration
	T2CONbits.T32 = 1;						// mode 32 bits : TMR3:TMR2
	T2CONbits.TCKPS = 0;					// prescaler 1:1 : 1 tick = 1/Fcy
	T2CONbits.TCS = 0;						// horloge interne Fcy

	// periode : 1 s -> PR3:PR2 = 29 999 999 (0x01C9C37F)
	prHigh = (uint16)(TIME_PR_MAX >> 16);	// poids forts
	prLow = (uint16)(TIME_PR_MAX & 0xFFFF);	// poids faibles
	PR3 = prHigh;
	PR2 = prLow;

	// remise a zero du compteur : MSB d'abord (TMR3HLD), LSB ensuite (TMR2)
	TMR3HLD = 0;
	TMR2 = 0;

	// compteur logiciel
	timeSeconds = 0;

	// interruption de debordement (le match 32 bits est signale par le timer 3)
	IPC2bits.T3IP = TIME_T3_PRIORITY;		// priorite
	IFS0bits.T3IF = 0;						// drapeau nettoye
	IEC0bits.T3IE = 1;						// interruption autorisee

	T2CONbits.TON = 1;						// demarrage
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TIMER 3 INTERRUPT
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Interruption Timer3 : le compteur 32 bits a atteint sa periode (1 s).
/// Incremente le compte de secondes. Le drapeau T3IF reste positionne tant que
/// cette interruption n'a pas ete executee : getTime() s'en sert pour corriger
/// la lecture quand l'increment n'est pas encore effectif (cf. getTime).
void __attribute__((__interrupt__, no_auto_psv)) _T3Interrupt(void) {
	IFS0bits.T3IF = 0;						// acquittement
	timeSeconds++;							// une seconde de plus
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GET TIME
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Retourne le temps systeme en microsecondes depuis la mise sous tension
/// (t = 0 = appel de Time_Init). Resolution : 1 us par LSB.
/// Erreur de datation < 5 us, independante de la charge d'interruptions :
/// la capture est coherente meme si l'interruption Timer3 est en retard.
/// Duree d'utilisation : 10 h = 3,6e10 us -> type Time 64 bits (cf. GenericTypeDefs.h).
/// Fonction reentrante, utilisable depuis le code projet et les interruptions.
Time getTime(void) {
	uint16 srSave;
	uint32 sec;
	uint16 tmrLow;
	uint16 tmrHigh;
	uint32 ticks;
	uint8 wrapPending;

	// ---- section critique : capture coherente de (secondes, timer, drapeau) ----
	// On monte la priorite CPU a 7 : aucune interruption utilisateur ne peut
	// s'inserer entre les lectures (duree totale ~15 cycles, soit 0,5 us).
	// Une simple relecture type seqlock serait insuffisante : si le timer boucle
	// entre la lecture du compteur et celle du drapeau, les valeurs capturees
	// sont incoherentes d'une seconde entiere.
	srSave = SR;							// sauvegarde de l'etat des interruptions
	SR |= 0x00E0;							// IPL = 7 : masque toutes les interruptions utilisateur

	sec = timeSeconds;
	// lecture coherente du timer 32 bits : lire TMR2 fige le poids fort dans TMR3HLD
	tmrLow = TMR2;
	tmrHigh = TMR3HLD;
	// debordement materiel survenu mais pas encore compte par l'interruption ?
	wrapPending = (uint8)IFS0bits.T3IF;

	if (wrapPending != 0) {
		// Drapeau leve : un debordement n'est pas encore compte. Mais est-il survenu
		// AVANT notre capture (-> il faut compter la seconde) ou PENDANT, entre la
		// lecture du compteur et celle du drapeau (-> le compteur capture est
		// pre-debordement : il ne faut PAS compter) ? ATTENTION : meme sous section
		// critique, le timer MATERIEL continue de tourner. Le compteur repond :
		// s'il est redescendu (valeur plus faible), le debordement date de notre
		// propre capture -> ne pas compter cette seconde.
		uint16 low2 = TMR2;
		uint16 high2 = TMR3HLD;
		uint32 ticks2 = ((uint32)high2 << 16) | low2;
		uint32 ticks1 = ((uint32)tmrHigh << 16) | tmrLow;
		if (ticks2 < ticks1) {
			wrapPending = 0;
		}
	}

	SR = srSave;							// restauration de l'etat des interruptions
	// ---- fin de section critique ----

	if (wrapPending != 0) {
		sec++;								// le timer a boucle depuis la lecture de sec
	}

	ticks = ((uint32)tmrHigh << 16) | tmrLow;

	// temps [us] = secondes x 1e6 + ticks / 30
	return ((Time)sec * (Time)1000000) + (Time)(ticks / TIME_TICKS_PER_US);
}
