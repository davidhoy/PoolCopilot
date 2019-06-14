/*
 * FirmwareUpdate.cpp
 *
 *  Created on: Nov 23, 2011
 *      Author: david
 */


#include "FirmwareUpdate.h"
#include <ftp.h>
#include <dhcpclient.h>
#include <bsp.h>
#include "Console.h"
#include "Utilities.h"
#include "StreamUpdate.h"
#include "Sha1.h"
#include "Watchdog.h"
#include "TinyXml/TinyXml.h"
#include "Poco/URI.h"

#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_FWUPDATE, level, fmt, ##__VA_ARGS__ )

extern DWORD FlashAppBase;
typedef int (*PFN_CALLBACK)(char *pszMsg);

#define USER_AGENT		"PoolCopilot"

static int ReadS19ApplicationCodeFromStreamEx( int fd,
		                                       const char *pszSha1,
		                                       PFN_CALLBACK pfnCallBack,
		                                       bool fTestMode );

static int ReadS19ApplicationCodeFromHttp( IPADDR       ipAddr,
									       const char   *host,
									       int          port,
									       const char   *file,
		                                   const char   *expectedSHA1,
		                                   PFN_CALLBACK pfnCallBack,
		                                   bool         fTestMode );

struct FlashStartUpStruct
{
      unsigned long dwBlockRamStart ;
      unsigned long dwExecutionAddr ;
      unsigned long dwBlockSize     ;
      unsigned long dwSrcBlockSize  ;
      unsigned long dwBlockSum      ;
      unsigned long dwStructSum     ;
};



FirmwareUpdate::FirmwareUpdate() :
	m_pszUrl( NULL ),
	m_IpAddr( 0 ),
	m_iPort( 21 ),
	m_pszUsername( NULL ),
	m_pszPassword( NULL ),
	m_iTimeout( 5 )
{
}

FirmwareUpdate::~FirmwareUpdate()
{
	if ( m_pszUrl )
	{
		free( m_pszUrl );
		m_pszUrl = NULL;
	}
	if ( m_pszUsername )
	{
		free( m_pszUsername );
		m_pszUsername = NULL;
	}
	if ( m_pszPassword )
	{
		free( m_pszPassword );
		m_pszPassword = NULL;
	}
}


int FirmwareUpdate::SetUrl( const char *pszUrl )
{
	int 		iRet = -1;
	Poco::URI	Url;
	IPADDR		IpAddr = 0;


	// Parse the URL.
	Url = pszUrl;

	// Use DNS to resolve IP address of the server
	//
	DebugTrace( 2, "Using DNS to get IP address of: %s...", Url.getHost().c_str()  );
	iRet = GetHostByName( Url.getHost().c_str() ,
						  &IpAddr,
						  EthernetDNS,
						  m_iTimeout * TICKS_PER_SECOND );
	if ( iRet != DNS_OK )
	{
		if ( iRet == DNS_TIMEOUT )
			DebugTrace( 2, "DNS Error: TIMEOUT\r\n" );
		else if ( iRet == DNS_NOSUCHNAME )
			DebugTrace( 2, "DNS Error: NO SUCH NAME\r\n" );
		else
			DebugTrace( 2, "DNS Error: %d\r\n", iRet );
	}
	else
	{
		DebugTrace( 2, "%s\r\n", IPToStr( IpAddr ) );

		if ( m_pszUrl )
			free( m_pszUrl );
		m_pszUrl       = strdup( Url.getHost().c_str() );
		m_iPort        = Url.getPort();
		m_pszDirectory = strdup( Url.getPath().c_str() );
		m_IpAddr       = IpAddr;
	} /* else */

	return iRet;
}



int FirmwareUpdate::SetCredentials( const char *pszUsername,
		                            const char *pszPassword )
{
	if ( m_pszUsername )
		free( (void*)pszUsername );
	m_pszUsername = strdup( pszUsername );

	if ( m_pszPassword )
		free( m_pszPassword );
	m_pszPassword = strdup( pszPassword );

	return 0;
}


/*-------------------------------------------------------------------
  This function reads the data stream from the fd and displays it to
  stdout, the debug serial port on the NetBurner device.
 -------------------------------------------------------------------*/
void ShowFileContents( int fdr )
{
	char tmp_resultbuff[255];

	Console.Printf( "\r\n[" );
	int rv;
	do
	{
		rv = ReadWithTimeout( fdr, tmp_resultbuff, 255, 20 );
		if ( rv < 0 )
		{
			Console.Printf( "RV = %d\r\n", rv );
		}
		else
		{
			tmp_resultbuff[rv] = 0;
			Console.Printf( "%s", tmp_resultbuff );
		}
	}
	while ( rv > 0 );
	Console.Printf( "]\r\n" );
}


int FirmwareStatusCallback( char *pszMsg )
{
	Console.Printf( "%s", pszMsg );
	return 0;
}


int FirmwareUpdate::DoUpdateVersion( const char *pszVersion,
				    				 bool fTestMode,
				                     bool fSupressReboot )
{
	int ret = -1;

	switch (m_iPort)
	{
	case 21:
		ret = DoUpdateVersionFtp(pszVersion, fTestMode, fSupressReboot);
		break;

	case 80:
		ret = DoUpdateVersionHttp(pszVersion, fTestMode, fSupressReboot);
		break;

	default:
		Console.Printf( "Error: no port specified\r\n");
		ret = -1;
	}

	return ret;
}


int FirmwareUpdate::DoUpdateVersionFtp( const char *pszVersion,
		                                bool fTestMode,
		                                bool fSupressReboot )
{
	char	*pszFileName = NULL;
	char	*pszSha1     = NULL;


	// Open FTP session at specified IP Address with specified user name
	// and password. There is a 5 second timeout.
	Console.Printf( "Connect to %s [%s]...\r\n", m_pszUrl, IPToStr(m_IpAddr) );
	int ftp = FTP_InitializeSession( m_IpAddr,
                                     m_iPort,
                                     m_pszUsername,
                                     m_pszPassword,
                                     m_iTimeout * TICKS_PER_SECOND );
	if ( ftp <= 0 )
	{
		Console.Printf( "Error: unable to connect to %s [%s], ret=%d\r\n",
			          	m_pszUrl, IPToStr(m_IpAddr), ftp );
		return 0;
	} /* if */


	// Change to correct directory
	Console.Printf( "Change directory to %s...\r\n", m_pszDirectory );
	int iRet = FTPSetDir( ftp, m_pszDirectory, m_iTimeout * TICKS_PER_SECOND );
	if ( iRet != FTP_OK )
	{
		Console.Printf( "Failed to change directory to %s, ret=%d\r\n",
				        m_pszDirectory, iRet );
		FTP_CloseSession( ftp );
		return 0;
	} /* if */


	// Read Firmware.XML from FTP server
	Console.Printf( "Get Firmware.XML...\r\n" );
	int fdr = FTPGetFile( ftp,
        		          "Firmware.xml",
            		      FALSE,
            		      m_iTimeout * TICKS_PER_SECOND );
    if ( fdr <= 0 )
    {
    	Console.Printf( "Unable to get Firmware.xml, ret=%d\r\n", fdr );
 		FTP_CloseSession( ftp );
 		return 0;
    } /* if */


    // Read content of XML file into buffer.
    Console.Printf( "Reading XML stream...\r\n" );
    int iSize = 4*1024;
	char *pszXmlStr = (char *)malloc( iSize );
	memset( pszXmlStr, 0, iSize );
	iRet = ReadWithTimeout( fdr, pszXmlStr, iSize, 5*TICKS_PER_SECOND );
	if ( iRet > 0 )
	{
		Console.Printf( "Got XML:\r\n==================\r\n%s\r\n==================\r\n", pszXmlStr );

		Console.Printf( "Scanning XML for build %s...\r\n", iRet, pszVersion );
		TiXmlDocument doc;
		doc.Parse( pszXmlStr, 0, TIXML_DEFAULT_ENCODING );
		TiXmlHandle docHandle( &doc );
		TiXmlElement* pBuilds = docHandle.FirstChild( "PoolCopilotFirmware" ).FirstChild( "Builds" ).ToElement();
		if ( pBuilds )
		{
			// Iterate through the "Builds" node of the XML document
			//
			TiXmlElement* pBuild = pBuilds->FirstChildElement();
			while ( pBuild )
			{
				const char *pszBuildVersion  = pBuild->Attribute( "VersionNumber" );
				const char *pszBuildFileName = pBuild->Attribute( "FileName" );
				const char *pszBuildSha1     = pBuild->Attribute( "SHA1" );
				Console.Printf( "  Build v%s, filename=%s, SHA1=%s\r\n",
						        pszBuildVersion, pszBuildFileName, pszBuildSha1 );
				if ( pszBuildVersion && (strcmp(pszVersion, pszBuildVersion) == 0) )
				{
					pszFileName = strdup( pszBuildFileName );
					pszSha1     = strdup( pszBuildSha1 );
					break;
				} /* if */
				pBuild = pBuild->NextSiblingElement();
			} /* while */
		} /* if */
		else
		{
			Console.Printf( "No builds found!\r\n" );
		} /* else */
	} /* if */
	else
	{
		Console.Printf( "Error reading XML stream!\r\n" );
	} /* if */

	// Read the command result code from the FTPGetFile command
	char zResultBuf[255];
	iRet = FTPGetCommandResult( ftp,
								zResultBuf,
								255,
								m_iTimeout * TICKS_PER_SECOND );
	if ( iRet != 226 )
	{
		Console.Printf( "Error Command result = %d %s\r\n", iRet, zResultBuf );
	}

	if ( pszXmlStr )
		free( pszXmlStr );
	FTP_CloseSession( ftp );


	// If we have a filename and SHA1, go ahead and do the update.
	Console.Printf( "Filename=%s, SHA1=%s\r\n", pszFileName, pszSha1 );
	if ( pszFileName )
	{
		Console.Printf( "Do update with file %s, SHA1 %s\r\n", pszFileName, pszSha1 );
		DoUpdateFile( pszFileName, pszSha1, TRUE, TRUE );//fTestMode, fSupressReboot );
	} /* if */
	else
	{
		Console.Printf( "Unable to find specified version '%s'\r\n", pszVersion );
	}
	if ( pszFileName )	free( pszFileName );
	if ( pszSha1     )  free( pszSha1 );

	return 0;
}


int FirmwareUpdate::DoUpdateVersionHttp( const char *pszVersion,
		                                 bool fTestMode,
		                                 bool fSupressReboot )
{
	Console.Printf( "Error: not implemented yet\r\n");
	return 0;
}


int FirmwareUpdate::DoUpdateFile( const char *pszFileName,
		                          const char *pszSha1,
		                          bool fTestMode,
		                          bool fSupressReboot )
{
	int ret = -1;

#ifdef _DEBUG
	Console.Printf("\r\n%s: enter, filename=%s, sha1=%s, testmode=%d, noreboot=%d, port=%d\r\n", __func__,
			       pszFileName, pszSha1, fTestMode, fSupressReboot, m_iPort);
#endif

	switch (m_iPort)
	{
	case 21:
		ret = DoUpdateFileFtp(pszFileName, pszSha1, fTestMode, fSupressReboot);
		break;

	case 80:
		ret = DoUpdateFileHttp(pszFileName, pszSha1, fTestMode, fSupressReboot);
		break;

	default:
		Console.Printf( "Error: no port specified\r\n");
		ret = -1;
	}

#ifdef _DEBUG
	Console.Printf("\r\n%s: leave, ret=%d", __func__, ret);
#endif
	return ret;
}


int FirmwareUpdate::DoUpdateFileFtp( const char *pszFileName,
		                             const char *pszSha1,
		                             bool fTestMode,
		                             bool fSupressReboot )
{
	int		iRet = 0;
	char tmp_resultbuff[255];

	// Open FTP session at specified IP Address with specified user name
	// and password. There is a 5 second timeout.
	Console.Printf( "Connect to %s [%s]...\r\n", m_pszUrl, IPToStr(m_IpAddr) );
	int ftp = FTP_InitializeSession( m_IpAddr,
                                     m_iPort,
                                     m_pszUsername,
                                     m_pszPassword,
                                     m_iTimeout * TICKS_PER_SECOND );
	if ( ftp > 0 ) // if the var ftp is > 0, it is the session handle
	{
		int rv = 0;

		// Change to the test directory
		//************************************WARNING***************************
		// To run this sample a test1 directory must exist on the test server.
		//************************************WARNING***************************

		// Change to FIRMWARE directory
		rv = FTPSetDir( ftp, "firmware", m_iTimeout * TICKS_PER_SECOND );
		if ( rv == FTP_OK )
		{
			Console.Printf( "Reading %s from server...\r\n", pszFileName );
            int fdr = FTPGetFile( ftp,
            		              pszFileName,
            		              FALSE,
            		              m_iTimeout * TICKS_PER_SECOND );
            if ( fdr > 0 )
            {
            	// Stop the watchdog so we don't reset in the middle of writing to
				// the flash
				g_Watchdog.Stop();

            	iRet = ReadS19ApplicationCodeFromStreamEx( fdr, pszSha1, FirmwareStatusCallback, fTestMode );
            	close( fdr );
            	if ( (iRet == STREAM_UP_OK) && (fSupressReboot == false) )
            	{
            		Console.Printf( "Rebooting..." );
            		OSTimeDly( 2 * TICKS_PER_SECOND );
            		ForceReboot();
            	} /* if */

            	g_Watchdog.Start( 5000 );


            	// Read the command result code from the FTPGetFile command
            	rv = FTPGetCommandResult( ftp,
            			                  tmp_resultbuff,
            			                  255,
            			                  m_iTimeout * TICKS_PER_SECOND );
            	if ( rv != 226 )
            	{
            		Console.Printf( "Error Command result = %d %s\r\n", rv, tmp_resultbuff );
            	}
            }
            else
            {
            	Console.Printf( "Failed to get file %s\r\n", pszFileName );
            }
        }
		else
        {
			Console.Printf( "Failed to change to FIRMWARE directory\r\n" );
        }

		FTP_CloseSession( ftp );
	}
	else
	{
		Console.Printf( "Failed to open FTP Session\r\n" );
	}

	return 0;

}


off_t getSizeOfFile(const char   *host,
  	                IPADDR       ipAddr,
		            int          port,
		            const char   *file,
		            PFN_CALLBACK callback)
{
	char msg[80];

#ifdef DEBUG

	sprintf(msg, "%s: enter\r\n", __func__);
	callback(msg);
	callback( "Connecting to server...");
#endif

    int sock = connect(ipAddr, 0, port, 5 * TICKS_PER_SECOND);
    if (sock < 0)
    {
        sprintf(msg, "Could not connect, error=%d\r\n", sock);
        callback(msg);
        return sock;
    }
#ifdef _DEBUG
    callback("connected\r\n");
#endif

    const int bufsize = 1024;
    char *buffer = (char *)malloc(bufsize);
    if (buffer == NULL)
    {
    	callback("Out of memory\r\n");
    	close(sock);
    	return -1;
    }
    sprintf(buffer, "GET %s HTTP/1.1\r\n"
                    "Connection: keep-alive\r\n"
                    "Host: %s:%d\r\n"
                    "Range: bytes=0-8\r\n"
                    "Accept: text/html,application/xhtml+xml,application/xml,*/*\r\n"
                    "User-Agent: %s\r\n\r\n",
                    file, host, port, USER_AGENT);
    ssize_t bytesToWrite = strlen(buffer);
    char *ptr = buffer;

#ifdef _DEBUG
    callback(buffer);
    callback("Sending request...");
#endif

    do {
        ssize_t bytesWritten = write(sock, ptr, bytesToWrite);
        if (bytesWritten < 0)
        {
            callback("Error writing request to socket\r\n");
            free(buffer);
            return -1;
        }
        if (bytesWritten == 0)
        {
            break;
        }
        bytesToWrite -= bytesWritten;
        ptr += bytesWritten;
    } while (bytesToWrite > 0);

#ifdef _DEBUG
    callback("done\r\n");
    callback("Reading response...");
#endif

    ssize_t bytesRead = read( sock, buffer, bufsize);
    if (bytesRead <= 0 )
    {
    	callback("Error reading from socket\r\n");
    	close(sock);
    	free(buffer);
        return -1;
    }
#ifdef _DEBUG
    callback("done\r\n");
    callback(buffer);
#endif

    off_t fileLength = 0;
    off_t start,end;
    const char *str = "Content-Range: bytes ";
    ptr = strstr(buffer, str);
    if (ptr != NULL)
    {
        sscanf(ptr+strlen(str), "%ld-%ld/%ld", &start, &end, &fileLength);
    }

#ifdef _DEBUG
    sprintf(msg, "%s: leave, fileLength=%ld\r\n", __func__, fileLength);
    callback(msg);
#endif

    free(buffer);
    close(sock);
    return fileLength;
}


off_t getRangeOfFile(const char      *host,
		               IPADDR        ipAddr,
                       int           port,
                       const char    *file,
                       off_t         offset,
                       off_t         length,
                       unsigned char *buffer,
                       PFN_CALLBACK  callback)
{
	char msg[130];

#ifdef _DEBUG
	sprintf(msg, "%s: enter\r\n", __func__);
	callback(msg);
#endif

    int sock = connect(ipAddr, 0, port, 5*TICKS_PER_SECOND);
    if (sock < 0)
    {
       sprintf(msg, "Could not connect, error=%d\r\n", sock);
       callback(msg);
       return -1;
    }

    char *request = (char *)malloc(1024);
    if (request == NULL)
    {
    	callback("Error allocating memory\r\n");
    	return -1;
    }
    sprintf(request, "GET %s HTTP/1.1\r\n"
                     "Connection: keep-alive\r\n"
                     "Host: %s:%d\r\n"
                     "Range: bytes=%lu-%lu\r\n"
                     "Accept: text/html,application/xhtml+xml,application/xml,*/*\r\n"
                     "User-Agent: %s\r\n\r\n",
                     file, host, port, offset, offset+length-1, USER_AGENT );
    ssize_t bytesToWrite = strlen(request);
    char *ptr = request;
    do {
        ssize_t bytesWritten = write(sock, ptr, bytesToWrite);
        if (bytesWritten < 0)
        {
            callback("Error writing request to socket\r\n");
            free(request);
            close(sock);
            return -1;
        }
        if (bytesWritten == 0)
        {
            break;
        }
        bytesToWrite -= bytesWritten;
        ptr += bytesWritten;
    } while (bytesToWrite > 0);
    free(request);

    ssize_t totalBytes = 0;
    ssize_t bytesRead;
    char *response = (char *)malloc(length+1024);
    ptr = response;
    do
    {
        bytesRead = read(sock, ptr, 512);
        if (bytesRead < 0 )
        {
        	sprintf(msg, "Could not read, error=%d\r\n", bytesRead);
        	callback(msg);
        	free(response);
            close(sock);
            return -1;
        }
        totalBytes += bytesRead;
        ptr += bytesRead;
    } while (bytesRead >= 512);

    ptr = strstr(response, "\r\n\r\n");
    memcpy(buffer, ptr+4, length);
    totalBytes -= (ptr - response + 4);

    free(response);
    close(sock);
    return totalBytes;
}


int FirmwareUpdate::DoUpdateFileHttp( const char *pszFileName,
		                              const char *pszSha1,
		                              bool fTestMode,
		                              bool fSupressReboot )
{
	int iRet = 0;

#ifdef _DEBUG
	Console.Printf("\r\n%s: enter, filename=%s, sha1=%s, url=%s, port=%d, ipaddr=%s\r\n", __func__,
			       pszFileName, pszSha1, m_pszUrl, m_iPort, IPToStr(m_IpAddr));
#endif

	Console.Printf( "Connect to %s...\r\n", m_pszUrl );

	// Stop the watchdog so we don't reset in the middle of writing to
	// the flash
	g_Watchdog.Stop();

	// Reprogram flash from HTTP stream
	iRet = ReadS19ApplicationCodeFromHttp(m_IpAddr,
			                              m_pszUrl,
			                              m_iPort,
			                              pszFileName,
			                              pszSha1,
			                              FirmwareStatusCallback,
			                              fTestMode);
	if (iRet != STREAM_UP_OK)
	{
		Console.Printf("\r\nHTTP firmware updated failed\r\n");
	}
	else
	{
		if (fSupressReboot == false && fTestMode == false)
		{
			Console.Printf( "\r\nFirmware updated, rebooting..." );
			OSTimeDly( 2 * TICKS_PER_SECOND );
			ForceReboot();
		} /* if */
	}

   	g_Watchdog.Start( 5000 );
	return 0;
}


#define USER_FLASH_SIZE (8192)
BYTE _user_flash_buffer[USER_FLASH_SIZE];


static int ReadS19ApplicationCodeFromStreamEx( int fd,
		                                       const char *pszSha1,
		                                       PFN_CALLBACK pfnCallBack,
		                                       bool fTestMode )
{
	int rv;
	int nchars = 0;
	int nlines = 0;

	DWORD addr;
	unsigned char * CopyTo = NULL;
	DWORD cur_pos;
	DWORD maxlen;

	char Rx_buffer[255];
	char Rx_S19_Line[255];
	BOOL bAlloced = FALSE;
	CopyTo = _user_flash_buffer;
	addr = (DWORD)&FlashAppBase;
	maxlen = USER_FLASH_SIZE;
	cur_pos = 0;
	int iRet = STREAM_UP_FAIL;

	CSHA1	sha1;
	char	szTemp[128];

	if ( pfnCallBack == NULL )
	{
		return STREAM_UP_FAIL;
	}

	pfnCallBack( "Parsing code update stream..." );
	char *cp = Rx_S19_Line;
	do
	{
		rv = ReadWithTimeout( fd, Rx_buffer, 255, 5 * TICKS_PER_SECOND );
		if ( rv > 0 )
		{
			sha1.Update( (unsigned char *)Rx_buffer, rv );
			for ( int i = 0; i < rv; i++ )
			{
				if ( Rx_buffer[i] != '\r' )
				{
					if ( Rx_buffer[i] == '\n' )
					{
						*cp = 0;
						cp = Rx_S19_Line;
						nchars = 0;

						if ( ProcessS3( Rx_S19_Line, addr, CopyTo, cur_pos, maxlen ) != 0 )
						{
							if ( nlines % 5000 == 0 )
							{
								sprintf( szTemp, "\r\n%5d:", nlines);
								pfnCallBack( szTemp );
							}
							if ( nlines % 100 == 0 )
							{
								pfnCallBack( "." );
							}

							if ( nlines == 5 )
							{
								/* We have read enough lines to get the header for size */
								FlashStartUpStruct *ps = (FlashStartUpStruct *)_user_flash_buffer;
								if ( ( ps->dwBlockRamStart +
									   ps->dwExecutionAddr +
									   ps->dwBlockSize +
									   ps->dwSrcBlockSize +
									   ps->dwBlockSum +
									   ps->dwStructSum ) !=
									 0x4255524E )
								{
									//Structure fails checksum
									pfnCallBack( "\r\nInvalid Flash startup structure checksum\r\n" );
									return STREAM_UP_FAIL;
								}

								DWORD siz = ps->dwSrcBlockSize + 28;
								PBYTE pb = ( PBYTE ) malloc( siz );
								if ( pb == NULL )
								{
									pfnCallBack( "memory allocation failed\r\n" );
									return STREAM_UP_FAIL;
								}
								/* Now copy what we have */
								memcpy( pb, _user_flash_buffer, cur_pos );
								CopyTo = pb;
								bAlloced = TRUE;
								maxlen = siz;
							}
							nlines++;
						}
					}
					else
					{
						*cp++ = Rx_buffer[i];
						nchars++;
						if ( nchars > 254 )
						{
							pfnCallBack( "internal buffer overflow\r\n" );
							return STREAM_UP_FAIL;
						}
					}
				}
			}
		}
	} while ( rv > 0 );

	if ( cur_pos == ( maxlen - 4 ) )
	{
		TCHAR   szSha1[60];
		sha1.Final();
		sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );

		if ( pszSha1 != NULL )
		{
			if ( stricmp( pszSha1, szSha1 ) == 0 )
			{
				pfnCallBack( "\r\nSHA1 checksums match!\r\n" );
			}
			else
			{
				sprintf( szTemp, "\r\nSHA1 checksum FAILED!"
    	 			             "\r\nComputed SHA1: %s"
    				             "\r\nExpected SHA1: %s\r\n",
    				             szSha1, pszSha1 );
				pfnCallBack( szTemp );
				iRet = STREAM_UP_FAIL;
				goto exit;
			}
		}
		else
		{
			pfnCallBack( "\r\nSkipping SHA1 check\r\n" );
		}

		pfnCallBack( "Writing code to flash..." );
		USER_ENTER_CRITICAL();
		FlashErase( ( void * ) addr, cur_pos );
		FlashProgram( ( void * ) addr, ( void * ) CopyTo, cur_pos );
		USER_EXIT_CRITICAL();
		pfnCallBack( "done\r\n" );

		iRet = STREAM_UP_OK;
	}

exit:
	if ( bAlloced )
	{
		free( CopyTo );
	}
	return iRet;
}


static int ReadS19ApplicationCodeFromHttp( IPADDR       ipAddr,
									       const char   *host,
									       int          port,
									       const char   *file,
		                                   const char   *expectedSHA1,
		                                   PFN_CALLBACK pfnCallBack,
		                                   bool         fTestMode )
{
	if ( pfnCallBack == NULL )
	{
		return STREAM_UP_FAIL;
	}

	DWORD addr;
	unsigned char * CopyTo = NULL;
	DWORD cur_pos;
	DWORD maxlen;

	char msg[130];

#ifdef _DEBUG
	sprintf(msg, "\r\n%s: enter, ipAddr=%s, host=%s, port=%d, file=%s, sha1=%s\r\n",
			__func__, IPToStr(ipAddr), host, port, file, expectedSHA1);
	pfnCallBack(msg);
#endif

	BOOL bAlloced = FALSE;
	CopyTo = _user_flash_buffer;
	addr = (DWORD)&FlashAppBase;
	maxlen = USER_FLASH_SIZE;
	cur_pos = 0;
	int iRet = STREAM_UP_FAIL;

	CSHA1	sha1;

	// Get the size of the file
#ifdef _DEBUG
	pfnCallBack( "Getting file size...");
#endif
	off_t fileSize = getSizeOfFile(host, ipAddr, port, file, pfnCallBack);
	if (fileSize < 0)
	{
		pfnCallBack( "Error: unable to get file size\r\n");
		return STREAM_UP_FAIL;
	}
#ifdef _DEBUG
	sprintf(msg, "length=%ld\r\n", fileSize);
	pfnCallBack(msg);
#endif

	pfnCallBack( "Parsing code update from HTTP...\n" );

	char line[80] = {0};
	int  lineIndex = 0;
	int  lineNum = 0;

	off_t totalBytesRead = 0;
	off_t start = 0;
	off_t block = 0;
	pfnCallBack("\r\n");
	while (totalBytesRead < fileSize)
	{
	    unsigned char buffer[2*1024];

	    sprintf(msg, "%ld: Reading bytes %ld-%ld of %ld...", block++, start, start+sizeof(buffer)-1, fileSize);
	    pfnCallBack(msg);

	    off_t bytesRead = getRangeOfFile(host,
	    		                         ipAddr,
	    		                         port,
	    		                         file,
	    		                         start,
	    		                         sizeof(buffer),
	    		                         buffer,
	    		                         pfnCallBack);
	    if (bytesRead > 0)
	    {
	        sprintf(msg, "ok, read %ld bytes\r\n", bytesRead);
	        pfnCallBack(msg);

	        sha1.Update( buffer, bytesRead );
	        for (int i = 0; i < bytesRead; i++)
	        {
	        	if (buffer[i] == '\r' || buffer[i] == '\n')
	        	{
	        		if (lineIndex > 0)
	                {
	                    line[lineIndex] = '\0';
	                    if ( ProcessS3( line, addr, CopyTo, cur_pos, maxlen ) != 0 )
	                    {
#if 0
	                    	if ( lineNum % 5000 == 0 )
	                    	{
	                    		sprintf( msg, "\r\n%5d:", lineNum);
	                    		pfnCallBack( msg );
	                    	}
	                    	if ( lineNum % 100 == 0 )
	                    	{
	                    		pfnCallBack( "." );
	                    	}
#endif
	                    	if ( lineNum == 5 )
	                    	{
	                    		/* We have read enough lines to get the header for size */
	                    		FlashStartUpStruct *ps = (FlashStartUpStruct *)_user_flash_buffer;
	                    		if ( ( ps->dwBlockRamStart +
	                    			   ps->dwExecutionAddr +
	                    			   ps->dwBlockSize +
	                    			   ps->dwSrcBlockSize +
	                    			   ps->dwBlockSum +
	                    			   ps->dwStructSum ) != 0x4255524E )
	                    		{
	                    			//Structure fails checksum
	                    			pfnCallBack( "\r\nInvalid Flash startup structure checksum\r\n" );
	                    			return STREAM_UP_FAIL;
	                    		}
	                    		DWORD siz = ps->dwSrcBlockSize + 28;
	                    		PBYTE pb = ( PBYTE ) malloc( siz );
	                    		if ( pb == NULL )
	                    		{
	                    			pfnCallBack( "memory allocation failed\r\n" );
	                    			return STREAM_UP_FAIL;
	                    		}
	                    		/* Now copy what we have */
	                    		memcpy( pb, _user_flash_buffer, cur_pos );
	                    		CopyTo = pb;
	                    		bAlloced = TRUE;
	                    		maxlen = siz;
	                    	}
	                    }
	                    lineNum++;
	                    lineIndex = 0;
	                }
	            }
	            else
	            {
	            	line[lineIndex++] = buffer[i];
	            }
	        }
	        totalBytesRead += bytesRead;
	        start += bytesRead;
	    }
	    else
	    {
	        int err = bytesRead;
	        sprintf(msg, "error %d-%s\n", err, strerror(err));
	        pfnCallBack(msg);
	        break;
	    }
	} /* while */


	if (cur_pos == ( maxlen - 4 ))
	{
		TCHAR   calculatedSHA1[60];
		sha1.Final();
		sha1.ReportHash( (TCHAR*)calculatedSHA1, CSHA1::REPORT_HEX_SHORT );

		sprintf(msg, "\r\nComputed SHA1: %s\r\n", calculatedSHA1);
		pfnCallBack(msg);

		if (expectedSHA1 != NULL)
		{
			sprintf(msg, "Expected SHA1: %s\r\n", expectedSHA1);
			pfnCallBack(msg);
			if ( stricmp( expectedSHA1, calculatedSHA1 ) == 0 )
			{
				pfnCallBack("SHA1 checksums match!\r\n" );
			}
			else
			{
				pfnCallBack("SHA1 checksum FAILED!\r\n" );
				iRet = STREAM_UP_FAIL;
				goto exit;
			}
		}
		else
		{
			pfnCallBack( "Skipping SHA1 check\r\n" );
		}

		if (fTestMode)
		{
			pfnCallBack("Test mode, not writing to flash.\r\n");
		}
		else
		{
			pfnCallBack( "Writing code to flash..." );
			USER_ENTER_CRITICAL();
			FlashErase( ( void * ) addr, cur_pos );
			FlashProgram( ( void * ) addr, ( void * ) CopyTo, cur_pos );
			USER_EXIT_CRITICAL();
			pfnCallBack( "done\r\n" );
		}
		iRet = STREAM_UP_OK;
	}

exit:
	if ( bAlloced )
	{
		free( CopyTo );
	}
	return iRet;
}

