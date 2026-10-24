///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		Main.c
///
///  @brief		Gère la boucle principale du main
///
///  @date		15/04/2014
///  @author	N/A
///  @copyright	SYSNAV
///////////////////////////////////////////////////////////////////////////////////////////////////



#include "Main.h"


int main()
{

	{
	// oscillator and specific boot
		systemPowerUp();

	// user init
		systemInit();

	// start devices
		systemStart();
	}

	// main loop
	while(1)
	{
		systemLoop();
	}

	// shut down
	systemShutDown();

	return RETURN_ERROR;
}//main

