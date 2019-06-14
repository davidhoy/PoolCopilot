/*
 * ServerInterface.h
 *
 *  Created on: Jun 22, 2011
 *      Author: david
 */

#ifndef SERVERINTERFACE_H_
#define SERVERINTERFACE_H_


#include "predef.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <startnet.h>
#include <iosys.h>
#include <utils.h>
#include <ip.h>
#include <tcp.h>
#include <string.h>
#include <dns.h>
#include <string>
#include "Poco/URI.h"
//#include <ucos.h>


class ServerInterface {

public:
	ServerInterface();
	~ServerInterface();

	void        SetContentType( std::string strContentType );
	std::string GetContentType( void )				{ return m_strContentType; }

	void SetTimeout( int iTimeout )					{ m_iTimeout = iTimeout; };
	int  GetTimeout( void )							{ return m_iTimeout; }

	int Connect( const char *pszUrl );
	int Close( void );

	int HttpPost( const char *pszExtraHeaders,
			      const int  iKeepAlive,
			      const char *pszPostData,
			      const int  cbPostData,
			      char       *pszPostResponse,
			      int        *pcbPostResponse,
			      const int  iHttpTimeout);
	void Abort( void )						{ m_bAbortFlag = TRUE; }

	//int	HttpGet( char *pszPath,
    //			     char *pszGetResponse,
	//		         int *pcbGetResponse );

	bool IsConnected( void )				{ return (m_fdServer > 0); }
	int  Socket( void )						{ return m_fdServer; }


private:

	Poco::URI		m_Url;
	std::string		m_strContentType;
	IPADDR 			m_ServerIp;

	int				m_fdServer;
	int				m_iTimeout;
	bool			m_bAbortFlag;

	int     SendRequest( int fd, char *pszBuffer, int cbBuffer );
	int		GetResponse( int fd, char *pszBuffer, int cbBuffer, int iTimeout );

};

#endif /* SERVERINTERFACE_H_ */
