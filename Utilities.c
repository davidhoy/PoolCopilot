/*
 * Utilities.c
 *
 *  Created on: Jul 19, 2011
 *      Author: david
 */

#include <stdio.h>
#include "Utilities.h"


#define HEXTOI( x )  ( (x >= 'A') ? (x - 'A' + 10) : (x - '0') )
#define ITOHEX( x )  ( (x  >  9 ) ? ('A' + x - 10) : ('0' + x) )


int IsChecksumValid( char *message )
{
    int             chksumValid = 0;
    unsigned char   msgChksum = 0;
    unsigned char   calcChksum = 0;
    unsigned char   *p = (unsigned char*)message;

    while ( *p != '*' && *p != '\r' && *p != '\0' )
    {
        calcChksum ^= *p++;
    }
    if ( *p++ == '*' )
    {
        msgChksum  = HEXTOI( *p ) << 4;
        p++;
        msgChksum |= HEXTOI( *p );
        if ( msgChksum == calcChksum )
        {
            chksumValid = 1;
        }
    }
    else
    {
        chksumValid = 1;
    }
    return chksumValid;
}


void AppendChecksum( char *message )
{
    unsigned char calcChksum = 0;
    unsigned char *p = (unsigned char*)message;
    unsigned char tmp;

    while ( *p != '\0' && *p != '\n' )
    {
        calcChksum ^= *p++;
    }
    *p++ = '*';
    tmp  = calcChksum >> 4;
    *p++ = ITOHEX( tmp );
    tmp  = calcChksum & 0x0F;
    *p++ = ITOHEX( tmp );
//    *p++ = '\r';
    *p   = '\0';
}


const char *MacToStr( unsigned char *mac )
{
	static char szBuffer[30];
	sprintf( szBuffer, "%02x:%02x:%02x:%02x:%02x:%02x",
             (int)mac[0], (int)mac[1], (int)mac[2],
             (int)mac[3], (int)mac[4], (int)mac[5] );
	return szBuffer;
}


const char * IPToStr( IPADDR ia )
{
	static char szBuffer[30];
	PBYTE ipb = (PBYTE) &ia;
	sprintf( szBuffer, "%d.%d.%d.%d",
			 (int)ipb[0], (int)ipb[1], (int)ipb[2], (int)ipb[3] );
	return szBuffer;
}


IPADDR StrToIP( const char *str )
{
	IPADDR	ia;
	PBYTE   ipb = (PBYTE) &ia;
	int		i1,i2,i3,i4;

	sscanf( str, "%d.%d.%d.%d", &i4, &i3, &i2, &i1 );
	ipb[0] = i4;
	ipb[1] = i3;
	ipb[2] = i2;
	ipb[3] = i1;
	return ia;
}


