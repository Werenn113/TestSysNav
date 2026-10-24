///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		Time.c
///  
///  @brief		Gestion du temps
///  
///  @date		15/04/2014
///  @author	N/A
///  @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////



#include "Time.h"
#include <Os/Os.h>
#include <xc.h>

// Compteur de secondes, volatile car modifié par une interruption
static volatile uint32_t system_seconds = 0;

void Time_Init(void) {
    T2CONbits.TON = 0;      // Stoppe le timer pour la configuration
    T2CONbits.T32 = 1;      // Active le mode 32 bits (Timer2 + Timer3)
    T2CONbits.TCKPS = 0;    // Prescaler 1:1 (Horloge à 30 MHz)
    T2CONbits.TCS = 0;      // Source d'horloge interne (Fcy)

    // Période réglée sur 30 000 000 (0x01C9C380) pour 1 seconde exacte
    PR3 = 0x01C9;           // Poids forts
    PR2 = 0xC380;           // Poids faibles

    TMR3 = 0;               // Remise à zéro
    TMR2 = 0;
    system_seconds = 0;

    IFS0bits.T3IF = 0;      // Efface le drapeau d'interruption du Timer3
    IEC0bits.T3IE = 1;      // Active l'interruption du Timer3

    T2CONbits.TON = 1;      // Démarre le chronomètre
}

// Routine d'interruption déclenchée toutes les secondes
void __attribute__((__interrupt__, no_auto_psv)) _T3Interrupt(void) {
    IFS0bits.T3IF = 0;      // Acquitte l'interruption
    system_seconds++;       // Incrémente le compteur de secondes
}

Time getTime(void) {
    uint32_t sec;
    uint32_t ticks;
    Time time_us;

    // Lecture atomique pour éviter une erreur si l'interruption 
    // se déclenche pendant la lecture des registres matériels.
    do {
        sec = system_seconds;
        
        // Sur le dsPIC, lire TMR2 (LSB) charge automatiquement 
        // le MSB dans le registre TMR3HLD pour figer sa valeur.
        uint16_t tmr2_val = TMR2; 
        uint16_t tmr3_val = TMR3HLD;
        
        ticks = ((uint32_t)tmr3_val << 16) | tmr2_val;
    } while (sec != system_seconds);

    // Calcul du temps en microsecondes :
    // Secondes * 1 000 000 + (ticks matériels / 30)
    time_us = ((Time)sec * 1000000ULL) + (ticks / 30);

    return time_us;
}