/*
 * Console.cpp
 *
 *  Created on: Jul 20, 2011
 *      Author: david
 */

#include "predef.h"
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <startnet.h>
#include <autoupdate.h>
#include <dhcpclient.h>
#include <smarttrap.h>
#include <taskmon.h>
#include <NetworkDebug.h>
#include <serial.h>
#include <bsp.h>
#include <command.h>
#include <iosys.h>
#include <string.h>
#include <stdarg.h>
#include "Console.h"
#include "Poco/URI.h"
#include "TaskPriorities.h"
#include "PoolCopInterface.h"
#include "PoolCopDataBindings.h"
#include "Utilities.h"
#include "FirmwareUpdate.h"
#include "Sha1.h"
#include "Version.h"
#include <list>
using std::list;


#define DNS_LOOKUP_TIMEOUT			(5 * TICKS_PER_SECOND)

typedef struct tagCMDTBL_ENTRY
{
	char        *pszCmd;
	PFN_CMDFUNC	pfnCmd;
	void 		*pContext;
	char		*pszHelpText;
} CMDTBL_ENTRY, *PCMDTBL_ENTRY;
static list <CMDTBL_ENTRY>	g_CmdList;

#define MAX_TOKENS	20

typedef struct tagDEBUG_MODULE
{
	char *pszName;
	int  iLevel;
} DEBUG_MODULE, *PDEBUG_MODULE;
#define MAX_DEBUG_MODULES 10
static bool         g_GlobalDebugEnable = false;
static DEBUG_MODULE g_DebugModules[MAX_DEBUG_MODULES] =
{
	{ "main",        0 },
	{ "poolcopilot", 0 },
	{ "server",      0 },
	{ "poolcop",     0 },
	{ "fwupdate",    0 },
	{ "watchdog",    0 },
	{ NULL,          0 },
	{ NULL,          0 },
	{ NULL,          0 },
	{ NULL,          0 }
};

extern bool g_fUsingDhcp;

/* The User Authentication callback */
int CConsole::ProcessAuth( const char *name, const char *pass )
{
	int iRet = CMD_FAIL;

	if ( (strcmp( name, "poolcopilot" ) == 0) &&
	 	 (strcmp( pass, "Left$hift99" ) == 0) )
    {
		iRet = CMD_OK;
    }
	return iRet;
}


/* The command processing callback */
int CConsole::ProcessCommand( const char *command, FILE *fp, void *pData )
{
	int iRet = CMD_OK;


	// Tokenize the command line
	//
	char    *copy = strdup( command );
	int		argc = 0;
	char    *argv[MAX_TOKENS] = { 0 };
	char *p = strtok( copy, " " );
	while ( (p != NULL) && (argc < MAX_TOKENS) )
	{
		//Console.Printf( "\r\n%2d: %p - '%s'", argc, p, p );
		argv[argc++] = p;
		p = strtok( NULL, " " );
	} /* while */
	if ( argc < 1 )
	{
		free( copy );
		return 0;
	}

#if 0
	Console.Printf( "\r\nargc = %d", argc );
	for ( int i = 0; i < MAX_TOKENS; i++ )
	{
		Console.Printf( "\r\nargv[%d] = '%s'", i, argv[i] != NULL ? argv[i] : "NULL" );
	}
#endif


	// Search list for matching command
	//
	bool fFound = false;
	list<CMDTBL_ENTRY>::const_iterator it;
	for( it = g_CmdList.begin(); it != g_CmdList.end(); ++it )
	{
		if ( strcmp( argv[0], it->pszCmd ) == 0 )
		{
			iRet = it->pfnCmd( argc, argv, it->pContext );
			fFound = true;
			break;
		}
	}
	if ( !fFound )
	{
		Console.Printf( "\r\nCommand \"%s\" not recognized", command );
		Console.Printf( "\r\nType ? or help for list of commands" );
	}

	free( copy );
	return iRet;
}


void * CConsole::ProcessConnect( FILE *fp )
{
   const char *prompt;

   if ( ( int ) ( fp->_file ) == ( SERIAL_SOCKET_OFFSET ) )
   {
      prompt = "Serial0";
   }
   else if ( ( int ) ( fp->_file ) == ( SERIAL_SOCKET_OFFSET + 1 ) )
   {
      prompt = "Serial1";
   }
   else
   {
      prompt = "Telnet";
   }

   fiprintf( fp, "\r\nWelcome to the PoolCopilot Console"
		         "\r\n=================================="
		         "\r\n"
				 "%s> ", prompt );

   return (void *)prompt;
}


void CConsole::ProcessPrompt( FILE *fp, void *pData )
{
	fiprintf( fp, "\r\n%s> ", pData );
}


void CConsole::ProcessDisconnect( FILE *fp, int cause, void *pData )
{
	switch ( cause )
	{
    case CMD_DIS_CAUSE_TIMEOUT:
    	fputs( "\nTimed out\n", fp );
    	break;

    case CMD_DIS_CAUSE_CLOSED :
    	fputs( "\nGoodBye\n", fp );
    	break;

    case CMD_DIS_SOCKET_CLOSED:
    	fputs( "Socket closed\n", fp );
    	break;

    case CMD_DIS_AUTH_FAILED:
    	fputs( "Authentication failed\n", fp );
    	break;
	}
}


CConsole::CConsole()
{
}


CConsole::~CConsole()
{
}


void CConsole::DebugTrace( int iModule, int iLevel, const char *pszFmt, ... )
{
	if ( g_GlobalDebugEnable && (iLevel <= g_DebugModules[iModule].iLevel) )
	{
#if 0
		struct tm now = *localtime(&(time_t){time( NULL )});
		char timestamp[40];
		sprintf( timestamp, "%04d/%02d/%02d-%02d:%02d:%02d ",
				 now.tm_year+1900, now.tm_mon+1, now.tm_mday,
				 now.tm_hour, now.tm_min, now.tm_sec );
		SendToAll( timestamp, strlen(timestamp), true );
#endif

		char szBuffer[1024];
		va_list	args;
		va_start( args, pszFmt );
		int iLen = vsprintf( szBuffer, pszFmt, args );
		va_end( args );
		SendToAll( szBuffer, iLen, true );
	} /* if */
}


void CConsole::DebugTraceStr( int iModule, int iLevel, const char *pszStr )
{
	if ( g_GlobalDebugEnable && (iLevel <= g_DebugModules[iModule].iLevel) )
	{
		int iLen = strlen( pszStr );
		SendToAll( (char *)pszStr, iLen, true );
	} /* if */
}


void CConsole::Printf( char *pszFmt, ... )
{
	char	szBuffer[1024];
	va_list	args;

	va_start( args, pszFmt );
	int iLen = vsprintf( szBuffer, pszFmt, args );
	va_end( args );
	SendToAll( szBuffer, iLen, true );
}


void CConsole::Initialize( int fdSerial )
{
#if 0
    #warning "Telnet authentication disabled!"
	CmdAuthenticateFunc = NULL;
#else
	CmdAuthenticateFunc = ProcessAuth;
#endif
	CmdCmd_func         = ProcessCommand;
	CmdConnect_func     = ProcessConnect;
	CmdPrompt_func      = ProcessPrompt;
	CmdDisConnect_func  = ProcessDisconnect;
	CmdIdleTimeout      = TICKS_PER_SECOND * 2000;
	Cmdlogin_prompt     = "Connecting to PoolCopilot.\r\n"
			              "Enter your login credentials...\r\n";

	CmdStartCommandProcessor( CONSOLE_PRIO );
	CmdAddCommandFd( fdSerial, TRUE, TRUE );
	CmdListenOnTcpPort( 23, 1, 5 );

    AddCommand( "?",        "Display help",                    CmdDisplayHelp,    NULL );
	AddCommand( "help",     "Display command list",            CmdDisplayHelp,    NULL );
	AddCommand( "logout",   "Disconnect console session",      CmdLogout,         NULL );
	AddCommand( "pump",     "Display/control pool pump",       CmdPump,           NULL );
	AddCommand( "aux",      "Display/control pool aux relays", CmdAux,            NULL );
	AddCommand( "debug",    "Control debug output",            CmdDebug,          NULL );
	AddCommand( "ipconfig", "Display network configuration",   CmdIpconfig,       NULL );
    AddCommand( "reboot",   "Reboot the bridge board",         CmdReboot,         NULL );
	AddCommand( "mem",      "Display memory status",           CmdShowMemory,     NULL );
    AddCommand( "fwupd",    "Update firmware from FTP site",   CmdUpdateFirmware, NULL );
    AddCommand( "sha1",     "Run SHA1 validation test",        CmdSha1Test,       NULL );
    AddCommand( "ver",      "Display software version info",   CmdVer,            NULL );
    AddCommand( "ping",     "Ping specified IP/name",          CmdPing,           NULL );
    AddCommand( "date",     "View/set the current date",       CmdDate,           NULL );
    AddCommand( "time",     "View/set the current time",       CmdTime,           NULL );
    AddCommand( "httpupd",  "Update firmware via HTTP",        CmdHttpUpdate,     NULL );
}


int CConsole::AddCommand( char *pszCmd, char *pszHelpText, PFN_CMDFUNC pfnCmd, void *pContext )
{
	CMDTBL_ENTRY	Entry;
	Entry.pszCmd      = pszCmd;
	Entry.pszHelpText = pszHelpText;
	Entry.pfnCmd      = pfnCmd;
	Entry.pContext    = pContext;
	g_CmdList.push_back( Entry );

	return 0;
}


int CConsole::CmdDisplayHelp( int argc, char *argv[], void *pContext )
{
	Console.Printf( "\r\n" );
	list<CMDTBL_ENTRY>::const_iterator it;
	for ( it = g_CmdList.begin(); it != g_CmdList.end(); ++it )
	{
		Console.Printf( "%-10s: %s\r\n", it->pszCmd, it->pszHelpText );
	}
	return 0;
}


int CConsole::CmdLogout( int argc, char *argv[], void *pContext )
{
	return CMD_CLOSE;
}


int CConsole::CmdPump( int argc, char *argv[], void *pContext )
{
	if ( argc <= 1 )
	{
		Console.Printf( "\r\nPump is %s", g_PoolCopData.dynamicData.PumpState ? "ON" : "OFF" );
		return 0;
	}

	Console.Printf( "\r\nTurning pump %s...", argv[1] );
	g_PoolCop.ControlPump( atoi(argv[1]) );
	return 0;
}

int CConsole::CmdAux( int argc, char *argv[], void *pContext )
{
	if ( argc <= 1 )
	{
		Console.Printf( "\r\nPump is %s", g_PoolCopData.dynamicData.PumpState ? "ON" : "OFF" );
		for ( int i = 1; i < AUX_COUNT; i++ )
		{
			Console.Printf( "\r\nAux%d is %s", i, g_PoolCopData.auxSettings.Aux[i].Status ? "ON" : "OFF" );
		}
		return 0;
	}

	int iAux = atoi( argv[1] );
	Console.Printf( "\r\nTurning Aux%d %s...", iAux, argv[2] );
	g_PoolCop.ControlAux( iAux, atoi(argv[2]) );
	return 0;

}


int CConsole::CmdDebug( int argc, char *argv[], void *pContext )
{
	switch ( argc )
	{
	case 1:
		// Display current debug levels
		//
		Console.Printf( "\r\nGlobal debug flag: %s"
				        "\r\nModule        Level"
				        "\r\n------------  -----", g_GlobalDebugEnable ? "ON" : "OFF" );
		for ( int i = 0; i < MAX_DEBUG_MODULES; i++ )
		{
			if ( g_DebugModules[i].pszName != NULL )
			{
				Console.Printf( "\r\n%-12s  %d",
						        g_DebugModules[i].pszName,
						        g_DebugModules[i].iLevel );
			}
		}
		Console.Printf( "\r\n" );
		break;

	case 2:
		if ( strcmp( argv[1], "on" ) == 0 )
			g_GlobalDebugEnable = true;
		else if ( strcmp( argv[1], "off" ) == 0 )
			g_GlobalDebugEnable = false;
		else
			Console.Printf( "\r\nUnrecognized parameter \"%s\"", argv[1] );
		break;

	case 3:
		for ( int i = 0; i <= MAX_DEBUG_MODULES; i++ )
		{
			if ( (g_DebugModules[i].pszName != NULL) &&
				 (stricmp( argv[1], g_DebugModules[i].pszName ) == 0 ) )
			{
				g_DebugModules[i].iLevel = atoi( argv[2] );
				Console.Printf( "\r\nSetting debug level for %s to %d.",
						        g_DebugModules[i].pszName,
							    g_DebugModules[i].iLevel );
				if ( g_DebugModules[i].iLevel > 0 )
				{
					g_GlobalDebugEnable = true;
				}
				break;
			}
			if ( i == MAX_DEBUG_MODULES )
			{
				Console.Printf( "\r\nUnknown debug module \"%s\"", argv[1] );
			} /* if */
		} /* for */
		break;

	default:
		break;
	} /* switch */

	return 0;
}


int CConsole::CmdIpconfig( int argc, char *argv[], void *pContext )
{
	 Console.Printf( "\r\n       DHCP: %s", g_fUsingDhcp ? "yes" : "no" );
	 Console.Printf( "\r\n IP Address: %s", IPToStr( EthernetIP ) );
	 Console.Printf( "\r\nSubnet Mask: %s", IPToStr( EthernetIpMask ) );
	 Console.Printf( "\r\n    Gateway: %s", IPToStr( EthernetIpGate ) );
	 Console.Printf( "\r\n DNS Server: %s", IPToStr( EthernetDNS ) );
	 Console.Printf( "\r\nMAC Address: %s", MacToStr( (unsigned char*)gConfigRec.mac_address ) );
	 return 0;
}


int CConsole::CmdReboot( int argc, char *argv[], void *pContext )
{
	Console.Printf( "\r\nRebooting now..." );
	OSTimeDly( TICKS_PER_SECOND );

	// Divide by zero forces a trap which reboots the bridge.
	//
	ForceReboot();
	return 0;
}


int CConsole::CmdShowMemory( int argc, char *argv[], void *pContext )
{
	struct mallinfo mi;
	mi = mallinfo();

	Console.Printf( "\r\n----- heap info -----\r\n" );
	Console.Printf( "     used : %d\r\n",      mi.uordblks );
	Console.Printf( "spaceleft : %d\r\n",      spaceleft() );
	Console.Printf( "     free : %d\r\n\r\n",  spaceleft() + mi.fordblks );

	return 0;
}


int CConsole::CmdSha1Test( int argc, char *argv[], void *pContext )
{
    const char szInput1[]          = "abc";
    const char szExpectedResult1[] = "A9993E364706816ABA3E25717850C26C9CD0D89D";
    const char szInput2[]          = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const char szExpectedResult2[] = "84983E441C3BD26EBAAE4AA1F95129E5E54670F1";

    CSHA1   sha1;
	TCHAR   szSha1[60];

    Console.Printf( "\r\n" );

    sha1.Reset();
	sha1.Update( (unsigned char *)szInput1, strlen(szInput1));
	sha1.Final();
	sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );
	Console.Printf( "SHA1(\"%s\") = \"%s\" - %s\r\n", szInput1, szSha1,
                    stricmp( szSha1, szExpectedResult1 ) == 0 ? "PASSED" : "FAILED" );
    sha1.Reset();
    sha1.Update( (unsigned char *)szInput2, strlen(szInput2));
	sha1.Final();
	sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );
	Console.Printf( "SHA1(\"%s\") = \"%s\" - %s\r\n", szInput2, szSha1,
                    stricmp( szSha1, szExpectedResult2 ) == 0 ? "PASSED" : "FAILED" );
	Console.Printf( "\r\n" );

	return 0;
}


int CConsole::CmdUpdateFirmware( int argc, char *argv[], void *pContext )
{
	char			szFileName[1024] = { 0 };
#ifdef SB72
	char 			*pszBaseName = "PoolCopilot_APP";
#else
	char 			*pszBaseName = "PoolCopilot_SB700EX_APP";
#endif

	//Console.Printf( "\r\nargc = %d", argc );
	//for ( int i = 0; i < argc; i++ )
	//{
	//	Console.Printf( "\r\nargv[%d] = '%s'", i, argv[i] ? argv[i] : "NULL" );
	//}

	if ( argc > 1 && argv[1] != NULL )
	{
		sprintf( szFileName, "%s_%s.s19", pszBaseName, argv[1] );
	}
	else
	{
		sprintf( szFileName, "%s.s19", pszBaseName );
	}

	Console.Printf( "\r\nStarting firmware update - %s...\r\n", szFileName );

	FirmwareUpdate	fwupd;
	fwupd.SetUrl( "ftp://poolcopilot.com/firmware" );
	fwupd.SetCredentials( "poolcop", "firmware" );
	fwupd.DoUpdateFile( szFileName, NULL, false, false );

	return 0;
}


int CConsole::CmdHttpUpdate( int argc, char *argv[], void *pContext )
{
	static const char *usage = "Usage: httpupd <url> [-t(est)] [-n(oreboot)]\r\n"
				        "   url       - required, full url to image file, e.g. 'http://www.poolcopilot.com/firmware/image.s19'\r\n"
				        "   -test     - optional, if set file is downloaded and verified, but not written to flash\r\n"
				        "   -noreboot - optional, do not reboot after writing to flash\r\n";

	if (argc < 2)
	{
		Console.Printf("\r\n%s", usage);
		return 0;
	}

	Poco::URI    Url;
	const char   *url      = NULL;
	const char   *filename = NULL;
	unsigned int port      = 0;
	int          testmode  = FALSE;
	int          noreboot  = FALSE;

	for (int i = 1; i < argc; i++)
	{
		if (i == 1)
		{
			try
			{
				url = argv[i];
				Url      = url;
				filename = Url.getPath().c_str();
				port     = Url.getPort();
				Console.Printf("url='%s', filename='%s', port=%d\r\n", url, filename, port);
			}
			catch(int exception)
			{
				Console.Printf("Invalid URL '%s'\r\n", url);
			}
			continue;
		}
		if ((stricmp(argv[i], "-test") == 0) || (stricmp(argv[i], "-t") == 0))
		{
			testmode = TRUE;
			continue;
		}
		if ((stricmp(argv[i], "-noreboot") == 0) || (stricmp(argv[i], "-n") == 0))
		{
			noreboot = TRUE;
			continue;
		}
	} /* for */

	if ((url      == NULL) || (strlen(url)      == 0) ||
	    (filename == NULL) || (strlen(filename) == 0) ||
	    (port     == 0))
	{
		Console.Printf("Please specify fully qualified URL to firmware image\r\n");
		return 0;
	}

	Console.Printf("\r\nStarting HTTP firmware update - %s...\r\n", Url.toString().c_str(), filename);

	FirmwareUpdate	fwupd;
	fwupd.SetUrl(url);
	fwupd.DoUpdateFile(filename, NULL, testmode, noreboot);
	return 0;
}


int CConsole::CmdVer( int argc, char *argv[], void *pContext )
{
	Console.Printf( "\r\n" );
	Console.Printf( "Software version : %s\r\n", VERSION_STRING );
	Console.Printf( "Build date       : %s\r\n", __DATE__ );
	Console.Printf( "Build time       : %s\r\n", __TIME__ );
	return 0;
}



int CConsole::CmdPing( int argc, char *argv[], void *pContext )
{
	char	*pszHost = NULL;
	IPADDR  addr_to_ping;
	int		iCount = 1;
	int		iTimeout = 5000;
	int		iRet = 0;
	#define MS_PER_TICK		(1000 / TICKS_PER_SECOND)


	// Display usage if no command line arguments.
	if ( argc <= 1 )
	{
		Console.Printf( "\r\nUsage: ping [-n count] [-w timeout] target_name\r\n" );
		Console.Printf( "\r\n" );
		Console.Printf( "Options:\r\n" );
		Console.Printf( "    -n count       Number of echo requests to send.\r\n" );
		Console.Printf( "    -w timeout     Timeout in milliseconds to wait for each reply.\r\n" );
		Console.Printf( "\r\n" );
		return 0;
	} /* for */

	// Parse the command line arguments.
	for ( int i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' )
		{
			switch ( argv[i][1] )
			{
			case 'n':
				iCount = atoi( &argv[i][2] );
				break;
			case 'w':
				iTimeout = atoi( &argv[i][2] );
				break;
			} /* switch */
		} /* if */
		else
		{
			pszHost = argv[i];
		} /* else */
	} /* for */

	// Resolve the hostname into an IP address.
	iRet = GetHostByName( pszHost, &addr_to_ping, EthernetDNS, DNS_LOOKUP_TIMEOUT );
	if ( iRet != 0 )
	{
		Console.Printf( "\r\nPing request could not find host %s. Please check the name and try again.\r\n",
				        pszHost );
		return 0;
	} /* if */

	// Ping the host
	Console.Printf( "\r\nPinging %s [%s] with 32 bytes:\r\n", pszHost, IPToStr(addr_to_ping) );
	for ( int i = 0; i < iCount; i++ )
	{
		iRet = GetHostByName( pszHost,
			   	              &addr_to_ping,
				              EthernetDNS,
				              DNS_LOOKUP_TIMEOUT );
		iRet = Ping( addr_to_ping,
			         i,    		    /*Id */
			         i, 			/*Seq */
			         (iTimeout / MS_PER_TICK) ); 	/*Max Ticks*/
		if ( iRet >= 0 )
		{
			Console.Printf( "Reply from %s: bytes=32 time=%dms\r\n",
					        IPToStr(addr_to_ping),
					        (iRet * MS_PER_TICK) );
		} /* if */
		else
		{
			Console.Printf("Request timed out.\r\n" );
		} /* else */
	} /* for */
	return 0;

} /* CConsole::CmdPing() */


int CConsole::CmdDate( int argc, char *argv[], void *pContext )
{
	static const char *WeekDays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	if ( argc <= 1 )
	{
		time_t RawTime;
		struct tm *TimeInfo;

		time( &RawTime );  					// Get system time in time_t format
		TimeInfo = localtime( &RawTime );  	// Convert to struct tm format
		Console.Printf( "\r\nCurrent date is: %s %02d/%02d/%04d\r\n",
						WeekDays[TimeInfo->tm_wday],
						TimeInfo->tm_mon+1,
						TimeInfo->tm_mday,
						TimeInfo->tm_year+1900 );
		return 0;
	}
	return 0;
}

int CConsole::CmdTime( int argc, char *argv[], void *pContext )
{
	if ( argc <= 1 )
	{
		time_t RawTime;
		struct tm *TimeInfo;

		time( &RawTime );  					// Get system time in time_t format
		TimeInfo = localtime( &RawTime );  	// Convert to struct tm format
		Console.Printf( "\r\nCurrent time is: %02d:%02d:%02d\r\n",
				        TimeInfo->tm_hour,
				        TimeInfo->tm_min,
				        TimeInfo->tm_sec );
		return 0;
	}
	return 0;
}
