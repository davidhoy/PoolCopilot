/*
 * FirmwareUpdate.h
 *
 *  Created on: Nov 23, 2011
 *      Author: david
 */

#ifndef FIRMWAREUPDATE_H_
#define FIRMWAREUPDATE_H_

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


class FirmwareUpdate
{
public:
	FirmwareUpdate();
	~FirmwareUpdate();

	int SetUrl( const char *pszUrl );
	int SetCredentials( const char *pszUsername, const char *pszPassword );
	int DoUpdateVersion( const char *pszVersion,
				         bool fTestMode = false,
				         bool fSupressReboot = false );
	int DoUpdateFile( const char *pszFilename,
			          const char *pszSha1 = NULL,
			          bool fTestMode = false,
			          bool fSupressReboot = false );

private:
	int DoUpdateVersionFtp( const char *pszVersion,
					        bool fTestMode = false,
					        bool fSupressReboot = false );
	int DoUpdateFileFtp( const char *pszFilename,
				         const char *pszSha1 = NULL,
				         bool fTestMode = false,
				         bool fSupressReboot = false );

	int DoUpdateVersionHttp( const char *pszVersion,
						     bool fTestMode = false,
					         bool fSupressReboot = false );
	int DoUpdateFileHttp( const char *pszFilename,
					      const char *pszSha1 = NULL,
			              bool fTestMode = false,
			  	          bool fSupressReboot = false );

	char 	*m_pszUrl;
	IPADDR	m_IpAddr;
	int		m_iPort;
	char	*m_pszDirectory;
	char	*m_pszUsername;
	char    *m_pszPassword;
	int		m_iTimeout;

};

#endif /* FIRMWAREUPDATE_H_ */
