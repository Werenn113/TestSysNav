/*********************************************************************
 *
 *                  Generic Typedef's
 *
 *********************************************************************
 * FileName:        Generic.h
 * Dependencies:
 * Processor:       dsPIC33F/PIC24H
 * 
 * Complier:        MPLAB C30 v1.31.00 or higher
 *                  MPLAB IDE v7.22.00 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ********************************************************************/
#ifndef _GENERIC_H_
#define _GENERIC_H_


typedef unsigned char       BYTE;               // 8-bit
typedef unsigned short int  WORD;               // 16-bit

typedef unsigned int        UINT;
typedef unsigned char       UINT8;              // other name for 8-bit integer
typedef unsigned short      UINT16;             // other name for 16-bit integer
typedef unsigned long       UINT32;             // other name for 32-bit integer


typedef enum { FALSE = 0, TRUE } BOOL;


#endif
