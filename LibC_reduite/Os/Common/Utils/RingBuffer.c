///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		RingBuffer.c
///  
/// @brief		Buffer circulaire multi-instances, sans ecrasement
///  
/// @date		15/04/2014
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <Os/Common/Utils/RingBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Nombre d'octets stockes, calcule par difference libre des index (modulo 65536).
/// Lecture coherente d'un unique consommateur/producteur ; toute valeur lue est
/// un etat valide du buffer (les index sont publishes apres la copie, cf. Write/Read).
/// @param rb : instance
/// @return nombre d'octets stockes (0 a size)
static uint16 RingBuffer_getCount(RingBuffer *rb) {
	uint16 write;
	uint16 read;

	write = rb->write;
	read = rb->read;

	return (uint16)(write - read);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// INIT
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Cf. RingBuffer.h
ReturnStatus RingBuffer_Init(RingBuffer *rb, uint8 *buffer, uint16 size) {
	// arguments invalides
	if (rb == 0) {
		return RETURN_ARG1_ERR;
	}
	if (buffer == 0) {
		return RETURN_ARG2_ERR;
	}
	// puissance de 2 et bornes : le masque binaire exige une puissance de 2,
	// les index libres exigent size <= 32768
	if ((size < RING_BUFFER_MIN_SIZE) || (size > RING_BUFFER_MAX_SIZE)
			|| ((size & (size - 1)) != 0)) {
		return RETURN_ARG3_ERR;
	}

	rb->buffer = buffer;
	rb->size = size;
	rb->write = 0;
	rb->read = 0;

	return RETURN_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// WRITE
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Cf. RingBuffer.h
/// Ordre des operations garantissant la coherence vis-a-vis du consommateur :
/// 1. place disponible verifiee (jamais d'ecrasement),
/// 2. donnees copiees via le pointeur volatile,
/// 3. index d'ecriture publie en dernier (apres la copie).
/// Sur un cœur unique, si l'interruption consommatrice demarre, elle ne peut voir le
/// nouvel index que si les donnees sont deja en memoire.
ReturnStatus RingBuffer_Write(RingBuffer *rb, const uint8 *data, uint16 length) {
	uint16 mask;
	uint16 write;
	uint16 i;
	volatile uint8 *dest;

	// arguments invalides
	if (rb == 0) {
		return RETURN_ARG1_ERR;
	}
	if (data == 0) {
		return RETURN_ARG2_ERR;
	}

	// SANS ECRASEMENT : on n'ecrit que si tout tient, sinon rejet propre
	if (length > RingBuffer_GetFreeSpace(rb)) {
		return RETURN_BUSY;
	}

	mask = (uint16)(rb->size - 1);
	write = rb->write;
	dest = (volatile uint8 *)rb->buffer;

	// copie (les ecritures via pointeur volatile ne seront pas deplacees par le
	// compilateur au-dela de la publication de l'index, contrairement a memcpy)
	for (i = 0; i < length; i++) {
		dest[write & mask] = data[i];
		write++;
	}

	// publication : le consommateur voit maintenant les donnees
	rb->write = write;

	return RETURN_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// READ
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Cf. RingBuffer.h
/// Symetrique de Write : copie puis publication de l'index de lecture. Le producteur
/// ne liberera la place qu'apres la copie effective, donc jamais de reecriture pendant
/// la lecture.
ReturnStatus RingBuffer_Read(RingBuffer *rb, uint8 *data, uint16 length) {
	uint16 mask;
	uint16 read;
	uint16 i;
	volatile uint8 *src;

	// arguments invalides
	if (rb == 0) {
		return RETURN_ARG1_ERR;
	}
	if (data == 0) {
		return RETURN_ARG2_ERR;
	}

	// lecture transactionnelle : on ne lit que si tout est disponible
	if (length > RingBuffer_GetCount(rb)) {
		return RETURN_BUSY;
	}

	mask = (uint16)(rb->size - 1);
	read = rb->read;
	src = (volatile uint8 *)rb->buffer;

	// copie
	for (i = 0; i < length; i++) {
		data[i] = src[read & mask];
		read++;
	}

	// publication : le producteur voit maintenant la place libre
	rb->read = read;

	return RETURN_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// GET COUNT / FREE SPACE
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Cf. RingBuffer.h
uint16 RingBuffer_GetCount(RingBuffer *rb) {
	if (rb == 0) {
		return 0;
	}
	return RingBuffer_getCount(rb);
}

/// Cf. RingBuffer.h
uint16 RingBuffer_GetFreeSpace(RingBuffer *rb) {
	if (rb == 0) {
		return 0;
	}
	return (uint16)(rb->size - RingBuffer_getCount(rb));
}
