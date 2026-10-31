///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		RingBuffer.h
///  
/// @brief		Buffer circulaire multi-instances, sans ecrasement
///  
/// @date		15/04/2014
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H



#include <Os/Common/Utils/GenericTypeDefs.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Taille maximale des buffers : 32768 octets.
/// Les index internes sont libres (uint16, jamais remis a zero) : le nombre d'octets
/// stockes se calcule par difference modulo 65536, ce qui impose taille <= 32768.
#define RING_BUFFER_MAX_SIZE		((uint16)32768)

/// Le buffer doit etre une puissance de 2 : la position memoire se calcule par
/// masque binaire (rapide, pas de modulo).
#define RING_BUFFER_MIN_SIZE		((uint16)2)

///////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Instance de buffer circulaire.
/// La structure et la zone memoire sont allouees par l'appelant (pas d'allocation
/// dynamique embarquee) : chaque instance est independante.
/// Contrat de concurrence : UN producteur (ecriture) + UN consommateur (lecture).
/// Ces deux contextes peuvent etre differents (ex : boucle principale / interruption)
/// sans protection supplementaire. Exemples d'emploi : voir note explicative.
typedef struct {
	uint8 *buffer;				///< zone memoire allouee par l'appelant (taille size octets)
	uint16 size;				///< capacite en octets, puissance de 2
	volatile uint16 write;		///< index libre d'ecriture (jamais remis a zero)
	volatile uint16 read;		///< index libre de lecture (jamais remis a zero)
}RingBuffer;

///////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Initialise une instance de buffer circulaire sur la zone memoire fournie.
/// @param rb : instance a initialiser
/// @param buffer : zone memoire de travail (allouee par l'appelant)
/// @param size : capacite en octets, puissance de 2 entre 2 et 32768
/// @return RETURN_SUCCESS ou RETURN_ARGn_ERR si arguments invalides
ReturnStatus RingBuffer_Init(RingBuffer *rb, uint8 *buffer, uint16 size);

/// Ecrit length octets dans le buffer. ECRASEMENT INTERDIT : si la place libre est
/// insuffisante, rien n'est ecrit et la fonction echoue (l'appelant peut reessayer).
/// Contexte : producteur uniquement (boucle principale par exemple).
/// @return RETURN_SUCCESS, RETURN_BUSY si place insuffisante, RETURN_ARGn_ERR si arguments invalides
ReturnStatus RingBuffer_Write(RingBuffer *rb, const uint8 *data, uint16 length);

/// Lit length octets du buffer et les retire. Si les donnees disponibles sont
/// insuffisantes, rien n'est lu et la fonction echoue (lecture transactionnelle).
/// Contexte : consommateur uniquement (interruption par exemple).
/// @return RETURN_SUCCESS, RETURN_BUSY si donnees insuffisantes, RETURN_ARGn_ERR si arguments invalides
ReturnStatus RingBuffer_Read(RingBuffer *rb, uint8 *data, uint16 length);

/// Nombre d'octets actuellement stockes
uint16 RingBuffer_GetCount(RingBuffer *rb);

/// Nombre d'octets encore ecrivables
uint16 RingBuffer_GetFreeSpace(RingBuffer *rb);


#endif
