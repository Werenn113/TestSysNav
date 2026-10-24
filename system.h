///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		System.h
///  
///  @brief		initialise tout le système
///  
///  @date		15/04/2014
///  @author	N/A
///  @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _SYSTEM_H
#define _SYSTEM_H


#include <Os/Os.h>
typedef struct
{
}Device;



typedef struct 
{

}SystemStatus;


typedef struct 
{
}Os;


typedef struct 
{
	Os		os;
	Device		dev;
	SystemStatus	status;
}System;


int8 systemPowerUp();
int8 systemInit();
int8 systemStart();
int8 systemLoop();
int8 systemShutDown();



#endif

