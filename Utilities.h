/*
 * Utilities.h
 *
 *  Created on: Jul 19, 2011
 *      Author: david
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <constants.h>
#include <nettypes.h>

#ifdef __cplusplus
extern "C"
{
#endif
	int IsChecksumValid( char *message );
	void AppendChecksum( char *message );
	const char *MacToStr( unsigned char *mac );
	const char *IPToStr( IPADDR ia );
	IPADDR StrToIP( const char *str );
#ifdef __cplusplus
}
#endif

#endif /* UTILITIES_H_ */
