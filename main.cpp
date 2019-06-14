/*
 * Main.cpp
 *
 *  Created on: Jul 2, 2011
 *      Author: david
 */

#include "predef.h"
#include <stdio.h>
#include <ctype.h>
#include <startnet.h>
#include <autoupdate.h>
#include <dhcpclient.h>
#include <smarttrap.h>
#include <taskmon.h>
#include <NetworkDebug.h>
#include <serial.h>
#include <command.h>
#include <iosys.h>
#include <string.h>
#include <ethernet.h>
#include <nbtime.h>
#include "PoolCop.h"
#include "PoolCopInterface.h"
#include "PoolCopilot.h"
#include "ConfigData.h"
#include "Utilities.h"
#include "TaskPriorities.h"
#include "Console.h"
#include "HtmlServices.h"
#include "Watchdog.h"
#include "DataWatchdog.h"
#include "Version.h"


typedef enum
{
	JUST_BOOTED = 0,
	CHECKING_FOR_DATA,
	WAIT_FOR_REFRESH,
	NORMAL_OPERATION
} RUN_STATE;

extern "C"
{
	void UserMain(void * pd);
}

#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_MAIN, level, fmt, ##__VA_ARGS__ )

const char * 		AppName="PoolCopilot";
static bool			g_fRunning = true;
PoolCopInterface	g_PoolCop;
ConfigData			g_ConfigData;
CConsole			Console;
bool				g_fUsingDhcp = false;
HtmlServices		g_HtmlServices;
PoolCopilot			g_PoolCopilot;



/*-------------------------------------------------------------------
  SetTimeNTP() - Acquires time from a NTP server and sets the system
  time. For this function to operate correctly you need to have:
  - Access to the Internet
  - Valid IP address, mask, gateway and DNS set on your NetBurner device
 -------------------------------------------------------------------*/
BOOL SetTimeNTP( void )
{
	if( !(EtherLink()) )
	{
		OSTimeDly( (WORD)(1.5 * TICKS_PER_SECOND) );
	}

	Console.Printf( "Acquiring time from NTP server... " );
	if ( SetTimeNTPFromPool() )
	{
		Console.Printf( "done\r\n" );
		return TRUE;
	}
	else
	{
		Console.Printf( "failed\r\n" );
		Console.Printf( "Verify you have correct IP address, mask, gateway AND DNS set.\r\n");
	}
	return FALSE;
}


void UserMain( void* pd )
{
    // Initialize TCP/IP stack
	//
    InitializeStack();
    if (EthernetIP == 0)
    {
    	g_fUsingDhcp = true;
    	GetDHCPAddress();
    }

    // Set main task priority
    //
    OSChangePrio(MAIN_PRIO);

    // Enable network firmware update
    //
    EnableAutoUpdate();

    // Start the HTTP server
    //
    StartHTTP();

    // Start the task monitor
    //
    EnableTaskMonitor();

#ifndef _DEBUG
    EnableSmartTraps();
    //OSShowTasksOnLeds = 1;
#endif

#ifdef _DEBUG
    InitializeNetworkGDB_and_Wait();
#endif


//    g_ConfigData.Initialize();


    // Open UART1 and assign to stdin, stdout and stderr
    //
    SerialClose( 0 );
    SerialClose( 1 );
    int fdDebug = OpenSerial( 1, 115200, 1, 8, eParityNone );
    ReplaceStdio( 0, fdDebug ); // stdin via UART 1
    ReplaceStdio( 1, fdDebug ); // std out via UART 1
    ReplaceStdio( 2, fdDebug ); // stderr via UART 1

    Console.Initialize( fdDebug );

    Console.Printf( "\r\n\r\nPoolCop Application v%s\r\n"
    	    	    "-----------------------------\r\n\r\n",
    	    	    VERSION_STRING );


    // Initialize persistent data store.  If reset was due to the watchdog
    // we need to increment the internal counter.
    //
    if ( g_Watchdog.IsResetDueToWD() )
    {
    	int iResetCounter = g_ConfigData.GetIntValue( "ResetCounter" );
    	iResetCounter++;
    	g_ConfigData.SetValue( "ResetCounter", iResetCounter );
        g_ConfigData.Save();
    } /* if */


    // Start the watchdog with a 5sec timeout
    //
    bool fWatchdogEnabled = (bool)g_ConfigData.GetIntValue( "WatchdogEnabled" );
    if ( fWatchdogEnabled )
    {
    	g_Watchdog.Start( 5000 );
    } /* if */


    // Sync time with NTP server pool
    //
    SetTimeNTP();

    g_HtmlServices.RegisterPost();

    // Create PoolCopInterface instance, and start the task.
    //
    Console.Printf( "Initializing PoolCop serial interface..." );
    g_DataWatchdog.Start();
    g_PoolCop.Startup();
    Console.Printf( "done\r\n" );

    Console.Printf( "Initializing PoolCopilot application..." );
    //PoolCopilot PoolCopilot;
    g_PoolCopilot.Startup();
    Console.Printf( "done\r\n" );

    int iDelay = 60;
    while ( g_fRunning )
    {
       	OSTimeDly( iDelay * TICKS_PER_SECOND );
    }

    g_PoolCopilot.Shutdown();
  	g_PoolCop.Shutdown();
  	g_ConfigData.Save();

} /* UserMain() */



void WriteHtmlVariable(int fd, float val)
{
	char String[40];
	sprintf( String, "%3.2f", val );
	write( fd, String, strlen(String) );
}

