/*
 * Version.h
 *
 *  Created on: Nov 29, 2011
 *      Author: david
 */

#ifndef VERSION_H_
#define VERSION_H_

#include "version.ver"

#define _STR(x)     #x
#define STR(x)      _STR(x)

#define VERSION_NUMBER      VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD
#ifdef _DEBUG
	#define VERSION_STRING  STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_BUILD) " (dbg)"
#else
	#define VERSION_STRING  STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_BUILD)
#endif
#define VERSION_COMPANY     "PCFR"
#define VERSION_COPYRIGHT   "(c) PCFR 2018"
#define VERSION_TIMESTAMP   __DATE__ " - " __TIME__


#endif /* VERSION_H_ */
