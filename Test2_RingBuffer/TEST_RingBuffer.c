///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file		TEST_RingBuffer.c
///  
/// @brief		Harnais de tests du module RingBuffer (execution sur hote PC, sans cible).
///  
///				Compilation : gcc -I LibC_reduite -o test TEST_RingBuffer.c LibC_reduite/Os/Common/Utils/RingBuffer.c
///				Execution : ./test  ->  "TOUS LES TESTS ONT PASSE" si succes.
///  
/// @date		15/04/2014
/// @author		Baptiste Marchand
/// @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Os/Common/Utils/RingBuffer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TEST_SIZE		64			// puissance de 2 petite pour forcer les enroulements
#define TEST_BIG_SIZE	4096

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T1 : INIT
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testInit(void) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];

	// arguments invalides
	assert(RingBuffer_Init(0, mem, TEST_SIZE) == RETURN_ARG1_ERR);
	assert(RingBuffer_Init(&rb, 0, TEST_SIZE) == RETURN_ARG2_ERR);
	assert(RingBuffer_Init(&rb, mem, 0) == RETURN_ARG3_ERR);
	assert(RingBuffer_Init(&rb, mem, 1) == RETURN_ARG3_ERR);			// < 2
	assert(RingBuffer_Init(&rb, mem, 63) == RETURN_ARG3_ERR);			// pas puissance de 2
	assert(RingBuffer_Init(&rb, mem, 100) == RETURN_ARG3_ERR);
	assert(RingBuffer_Init(&rb, mem, 32768 + 2) == RETURN_ARG3_ERR);	// > 32768 (non puissance de 2)
	assert(RingBuffer_Init(&rb, mem, 65536) == RETURN_ARG3_ERR);		// > 32768

	// cas valides
	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);
	assert(RingBuffer_GetCount(&rb) == 0);
	assert(RingBuffer_GetFreeSpace(&rb) == TEST_SIZE);

	printf("T1  init ....................... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T2 : FIFO
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testFifo(void) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];
	uint8 in[16];
	uint8 out[16];
	uint16 i;

	for (i = 0; i < 16; i++) in[i] = (uint8)i;

	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);
	assert(RingBuffer_Write(&rb, in, 16) == RETURN_SUCCESS);
	assert(RingBuffer_GetCount(&rb) == 16);
	assert(RingBuffer_Read(&rb, out, 16) == RETURN_SUCCESS);
	assert(memcmp(in, out, 16) == 0);
	assert(RingBuffer_GetCount(&rb) == 0);

	printf("T2  fifo ....................... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T3 : REMPLISSAGE COMPLET SANS ECRASEMENT
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testFullNoOverwrite(void) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];
	uint8 byte;
	uint8 value;
	uint16 expected;

	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);

	// remplissage exact jusqu'a la capacite maximale
	value = 0;
	while (RingBuffer_GetFreeSpace(&rb) > 0) {
		byte = value++;
		assert(RingBuffer_Write(&rb, &byte, 1) == RETURN_SUCCESS);
	}
	assert(RingBuffer_GetCount(&rb) == TEST_SIZE);

	// ecriture impossible : le contenu doit rester INCHANGE
	byte = 0xFF;
	assert(RingBuffer_Write(&rb, &byte, 1) == RETURN_BUSY);
	assert(RingBuffer_GetCount(&rb) == TEST_SIZE);

	// vidage avec controle integrale
	expected = 0;
	while (RingBuffer_GetCount(&rb) > 0) {
		assert(RingBuffer_Read(&rb, &byte, 1) == RETURN_SUCCESS);
		assert(byte == expected++);
	}

	printf("T3  plein / sans ecrasement .... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T4 : TRANSACTIONNALITE DES ECHECS
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testTransactionalFailure(void) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];
	uint8 in[24];
	uint8 out[64];
	uint16 countBefore;
	uint16 i;

	for (i = 0; i < 24; i++) in[i] = (uint8)(i + 1);

	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);
	assert(RingBuffer_Write(&rb, in, 24) == RETURN_SUCCESS);      // 24 stockes
	assert(RingBuffer_Write(&rb, in, 24) == RETURN_SUCCESS);      // 48 stockes, libre = 16
	countBefore = RingBuffer_GetCount(&rb);
	assert(countBefore == 48);

	// ecriture trop grande pour la place libre (24 > 16) : RIEN ne doit changer
	assert(RingBuffer_Write(&rb, in, 24) == RETURN_BUSY);
	assert(RingBuffer_GetCount(&rb) == countBefore);

	// lecture plus longue que le contenu : RIEN ne doit etre retire
	assert(RingBuffer_Read(&rb, out, countBefore + 1) == RETURN_BUSY);
	assert(RingBuffer_GetCount(&rb) == countBefore);

	// le contenu est intouchable : la sequence in, deux fois de suite
	assert(RingBuffer_Read(&rb, out, countBefore) == RETURN_SUCCESS);
	assert(memcmp(in, out, 24) == 0);
	assert(memcmp(in, out + 24, 24) == 0);

	printf("T4  echecs transactionnels ..... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T5 : FUZZ ALEATOIRE AVEC REFERENCE (enroulements des index)
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint32 testSeed = 12345;

static uint32 testRnd(void) {
	testSeed = testSeed * 1103515245u + 12345u;
	return (testSeed >> 16) & 0x7FFF;
}

static void testFuzz(uint16 iterations) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];
	static uint8 ref[65536];				// file de reference
	uint16 refCount = 0;
	uint8 wbuf[32];
	uint8 rbuf[32];
	uint16 i;
	uint16 j;
	uint16 len;
	uint8 value = 0;
	uint16 busy = 0;

	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);

	for (i = 0; i < iterations; i++) {
		if ((testRnd() & 1) == 0) {
			// production aleatoire
			len = (uint16)(1 + testRnd() % 20);
			for (j = 0; j < len; j++) wbuf[j] = value++;
			if (RingBuffer_Write(&rb, wbuf, len) == RETURN_SUCCESS) {
				for (j = 0; j < len; j++) ref[refCount++] = wbuf[j];
			} else {
				busy++;						// rejet PROPRE : etat inchange
				assert(RingBuffer_GetCount(&rb) == refCount);
			}
		} else {
			// consommation aleatoire
			len = (uint16)(1 + testRnd() % 20);
			if (RingBuffer_Read(&rb, rbuf, len) == RETURN_SUCCESS) {
				for (j = 0; j < len; j++) assert(rbuf[j] == ref[j]);
				memmove(ref, ref + len, (size_t)(refCount - len));
				refCount = (uint16)(refCount - len);
			} else {
				busy++;
				assert(RingBuffer_GetCount(&rb) == refCount);
			}
		}

		// invariants permanents
		assert(RingBuffer_GetCount(&rb) == refCount);
		assert(RingBuffer_GetFreeSpace(&rb) + RingBuffer_GetCount(&rb) == TEST_SIZE);
	}

	printf("T5  fuzz %5u iterations .. OK (%u rejets propres, index > 65535/enroules)\n",
			iterations, busy);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T7 : PRODUCTEUR/CONSOMMATEUR OCTET PAR OCTET (modele ISR UART)
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testByteProducerConsumer(void) {
	RingBuffer rb;
	static uint8 mem[TEST_SIZE];
	uint8 byte;
	uint8 next = 0;
	uint16 i;

	assert(RingBuffer_Init(&rb, mem, TEST_SIZE) == RETURN_SUCCESS);

	// alternance stricte write(1) / read(1) : simule l'ISR qui vide chaque octet emis
	for (i = 0; i < 20000; i++) {
		byte = (uint8)(next + 1);
		if (RingBuffer_Write(&rb, &byte, 1) == RETURN_SUCCESS) {
			next = byte;
		}
		assert(RingBuffer_Read(&rb, &byte, 1) == RETURN_SUCCESS);
		assert(byte == next);
	}
	assert(RingBuffer_GetCount(&rb) == 0);

	printf("T7  octet/octet (20000) ....... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TEST T8 : TAMPOI SDCARD PAR BLOCS DE 512
///////////////////////////////////////////////////////////////////////////////////////////////////

static void testSdBlocks(void) {
	RingBuffer rb;
	static uint8 mem[TEST_BIG_SIZE];
	static uint8 block[512];
	uint16 i;
	uint16 j;
	uint8 sample[12];
	uint8 expected;
	uint16 produced = 0;
	uint16 flushable;

	assert(RingBuffer_Init(&rb, mem, TEST_BIG_SIZE) == RETURN_SUCCESS);

	for (i = 0; i < 500; i++) {
		// production : echantillons de 12 octets (valeur incrementale)
		for (j = 0; j < 12; j++) sample[j] = (uint8)(produced++);
		if (RingBuffer_Write(&rb, sample, 12) == RETURN_SUCCESS) {
			// rien a faire : le fuzz verifie deja le rejet
		}

		// consommation : flush par blocs de 512 uniquement si complets
		while (RingBuffer_GetCount(&rb) >= 512) {
			assert(RingBuffer_Read(&rb, block, 512) == RETURN_SUCCESS);
		}
	}

	// reste partiel : strictement < 512, jamais coupe
	flushable = RingBuffer_GetCount(&rb);
	assert(flushable < 512);

	// verif de coherence du reste
	expected = (uint8)(produced - flushable);
	while (RingBuffer_GetCount(&rb) > 0) {
		assert(RingBuffer_Read(&rb, block, 1) == RETURN_SUCCESS);
		assert(block[0] == expected);
		expected++;
	}

	printf("T8  blocs Sdcard 512/4096 .... OK\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////////////////////////////////////////////////////////////////////////////////////////////////

int main(void) {
	printf("=== TESTS RingBuffer (hote) ===\n");
	testInit();
	testFifo();
	testFullNoOverwrite();
	testTransactionalFailure();
	testFuzz(20000);
	testFuzz(20000);
	testByteProducerConsumer();
	testSdBlocks();
	printf("=== TOUS LES TESTS ONT PASSE ===\n");
	return 0;
}
