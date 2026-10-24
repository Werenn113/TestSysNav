///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		Peripheral.h
///  
///  @brief		Gestion des périphériques
///  
///  @author	N/A
///  @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _PERIPHERAL_H
#define _PERIPHERAL_H



#if defined (__PIC24F__)
	#include <PIC24F_plib.h>
#elif defined (__dsPIC33E__)
	#include <PIC33E_plib.h>
#elif defined (__dsPIC33F__)
	#include <PIC33F_plib.h>
#else
    #error Selected processor not supported
#endif



#endif //_PERIPHERAL_H
