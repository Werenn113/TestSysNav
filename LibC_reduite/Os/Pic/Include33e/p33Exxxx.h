///////////////////////////////////////////////////////////////////////////////////////////////////
///  @file		p33Exxx.h
///
///  @brief		Inclusion de l'header du pic
///
///  @date		01/09/2011
///  @author	Microchip
///  @copyright	Microchip
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__dsPIC33EP256MU806__)

#include <p33EP256MU806.h>

#elif defined(__dsPIC33EP256MU810__)

#include <p33EP256MU810.h>

#elif defined(__dsPIC33EP256MU814__)

#include <p33EP256MU814.h>

#elif defined(__dsPIC33EP512MU810__)

#include <p33EP512MU810.h>

#elif defined(__dsPIC33EP512MU814__)

#include <p33EP512MU814.h>

#else

#error -- processor ID not specified in generic header file

#endif
