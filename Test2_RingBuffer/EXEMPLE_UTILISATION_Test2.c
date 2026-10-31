///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		EXEMPLE_UTILISATION_Test2.c
///  
/// @brief		Exemples d'emploi du buffer circulaire dans le code projet :
///				1) trames NMEA produites a 4 Hz par le code utilisateur, videes par
///				   l'interruption TX d'un peripherique systeme (UART),
///				2) tampon d'ecriture de donnees binaires vers une Sdcard.
///				Fichier d'illustration : les fonctions notees PLACEHOLDER dependent du
///				code projet, les extraits se recopient dans les fichiers existants.
///  
/// @date		15/04/2014
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <xc.h>
#include <Os/Os.h>
#include <Os/Common/Utils/RingBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// PLACEHOLDERS DU CODE PROJET (a remplacer par vos fonctions reelles)
///////////////////////////////////////////////////////////////////////////////////////////////////

/// PLACEHOLDER : la carte Sdcard est-elle prete a recevoir une page ?
extern int8 sdReady(void);

/// PLACEHOLDER : ecrit une page de 512 octets sur la Sdcard, renvoie la taille ecrite
extern uint16 sdWriteBlock(const uint8 *block, uint16 length);

///////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Exemple 1 : buffer des trames NMEA (producteur = boucle principale a 4 Hz,
/// consommateur = interruption TX de l'UART). 512 octets >> une trame NMEA (~80 octets).
#define NMEA_BUFFER_SIZE		((uint16)512)

/// Exemple 2 : tampon d'ecriture Sdcard (producteur = acquisitions, consommateur =
/// tache de flush par blocs de 512 octets, la Sdcard ecrivant page par page).
#define SDC_BUFFER_SIZE		((uint16)4096)
#define SD_BLOCK_SIZE		((uint16)512)

///////////////////////////////////////////////////////////////////////////////////////////////////
// RAM ALLOCATION (cote projet : la LibC ne fait aucune allocation)
///////////////////////////////////////////////////////////////////////////////////////////////////

/// instances de buffers
static RingBuffer nmeaBuffer;
static RingBuffer sdBuffer;

/// zones memoire associees
static uint8 nmeaBufferMem[NMEA_BUFFER_SIZE];
static uint8 sdBufferMem[SDC_BUFFER_SIZE];

/// bloc temporaire de transfert vers la Sdcard
static uint8 sdBlock[SD_BLOCK_SIZE];

/// memo du demarrage de l'emission UART (ecrit par la boucle ET par l'IT -> volatile)
static volatile uint8 uartTxBusy = 0;

/// compteur applicatif de trames refusees (telemetrie projet)
static uint16 nmeaDropped = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALISATION (a appeler depuis systemInit, apres timeInit())
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Une ligne par instance, sans code de gestion interne : la memoire est fournie
/// par l'appelant, le module n'alloue rien.
void exempleBufferInit(void) {
	RingBuffer_Init(&nmeaBuffer, nmeaBufferMem, NMEA_BUFFER_SIZE);
	RingBuffer_Init(&sdBuffer, sdBufferMem, SDC_BUFFER_SIZE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXEMPLE 1 : PRODUCTEUR — trame NMEA formatee dans le code utilisateur, appele a 4 Hz
///////////////////////////////////////////////////////////////////////////////////////////////////

void userSendNmea(const uint8 *frame, uint16 length) {
	if (RingBuffer_Write(&nmeaBuffer, frame, length) != RETURN_SUCCESS) {
		// buffer sature : sans ecrasement, la trame est refusee proprement et
		// comptee comme perdue. (Choix applicatif : on peut aussi attendre.)
		nmeaDropped++;
		return;
	}

	// si l'emission est stoppee (l'IT se coupe quand le buffer est vide), on la relance
	if (uartTxBusy == 0) {
		uint8 firstByte;
		if (RingBuffer_Read(&nmeaBuffer, &firstByte, 1) == RETURN_SUCCESS) {
			IEC0bits.U1TXIE = 0;		// pas d'interruption pendant le demarrage
			U1TXREG = firstByte;		// declenche l'emission (vide U1TX -> IT TX)
			uartTxBusy = 1;
			IFS0bits.U1TXIF = 0;
			IEC0bits.U1TXIE = 1;		// la suite partira par interruptions
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXEMPLE 1 : CONSOMMATEUR — interruption TX de l'UART, vide le buffer octet par octet
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Le peripherique systeme (UART) est configure en emission par interruption : tant
/// que le buffer contient des donnees, chaque fin d'emission declenche cette
/// interruption qui alimente l'UART. Buffer vide -> arret propre de l'emission.
void __attribute__((__interrupt__, no_auto_psv)) _U1TXInterrupt(void) {
	uint8 byte;

	if (RingBuffer_Read(&nmeaBuffer, &byte, 1) == RETURN_SUCCESS) {
		U1TXREG = byte;				// encore des donnees : on enchaine
	} else {
		IEC0bits.U1TXIE = 0;		// buffer vide : plus rien a emettre
		uartTxBusy = 0;
	}

	IFS0bits.U1TXIF = 0;			// acquittement
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXEMPLE 2 : PRODUCTEUR — donnees binaires destinees a la Sdcard
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Un echantillon de journal : datation (fonctionnalite Test #1 : getTime) + valeur.
typedef STRUCT_PACKED {
	Time timestamp;
	uint16 value;
} LogSample;

void userLogSample(uint16 value) {
	LogSample sample;

	sample.timestamp = getTime();
	sample.value = value;

	// si le tampon est plein, l'echantillon est refuse (jamais ecrase) : la donnee
	// la plus ancienne reste intacte — comportement voulu pour un journal
	RingBuffer_Write(&sdBuffer, (const uint8 *)&sample, (uint16)sizeof(LogSample));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXEMPLE 2 : CONSOMMATEUR — flush vers la Sdcard quand la carte est prete
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Appele periodiquement depuis la boucle principale (ou une tache systeme) :
/// des que la carte est prete, le tampon est vide par blocs de 512 octets,
/// taille naturelle d'une page Sdcard : ecritures alignees et rapides.
void userSdFlush(void) {
	uint16 copied;

	if (!sdReady()) {
		return;						// carte absente ou occupee : le tampon tamponne
	}

	while (RingBuffer_GetCount(&sdBuffer) >= SD_BLOCK_SIZE) {
		RingBuffer_Read(&sdBuffer, sdBlock, SD_BLOCK_SIZE);
		copied = sdWriteBlock(sdBlock, SD_BLOCK_SIZE);
		if (copied != SD_BLOCK_SIZE) {
			// erreur carte : on arrete le flush, le tampon conserve les donnees
			return;
		}
	}

	// reste partiel : garde dans le tampon jusqu'a completion du bloc (jamais coupe)
}
