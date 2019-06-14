/*
 * Watchdog.cpp
 *
 *  Created on: Dec 3, 2011
 *      Author: david
 */

#include <stdio.h>
#include <basictypes.h>
#include <string.h>
#include "TaskPriorities.h"
#if defined( SB72 )
    #include "sim5272.h"
#elif defined( SB700EX )
    #include "sim5270.h"
#endif
#include "Watchdog.h"
#include "Console.h"
#include "ucos.h"


#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_WATCHDOG, level, fmt, ##__VA_ARGS__ )

#ifdef SB72
	static DWORD 	g_WatchdogTaskStack[USER_TASK_STK_SIZE];
#endif

Watchdog::Watchdog() :
	m_fEnabled( false ),
	m_fTestMode( false ),
	m_uTimeout( 0 ),
	m_uShortestTimeLeft( WATCHDOG_MAX_TIME ),
	m_fLastResetDueToWD( false ),
	m_fThreadRunning( false ),
	m_fShutdownThread( false )
{

	Console.AddCommand( "wd", "Display/control watchdog timer", CmdWatchdog, this );
	m_fLastResetDueToWD = IsResetDueToWD();
}


Watchdog::~Watchdog()
{
}



/********************************************************************
* Watchdog::Start()
*
*   This function enables the MCF5272 software watchdog timer. The
*   max time in milliseconds that the timer can be set is 16383ms
*   (16.8Sec).
*
* Parameters:
*	MaxTime_ms: Max time in ms before HWDG will reset CPU
*
* Return:
* 	void
*********************************************************************/
void Watchdog::Start( unsigned int MaxTime_ms )
{
#ifdef SB72
	// Create a low-priority thread to periodically kick the watchdog
	//
	if ( m_fThreadRunning == false )
	{
		int iRet = OSTaskCreate( TaskThreadWrapper,
								 (void*)this,
								 (void*)&g_WatchdogTaskStack[USER_TASK_STK_SIZE],
								 (void*)g_WatchdogTaskStack,
								 WATCHDOG_PRIO );
		if ( iRet != OS_NO_ERR )
		{
			DebugTrace( 1, "%s: Error starting thread, err=%d\n", __FUNCTION__, iRet );
		} /* if */
	} /* if */

    // Each counter tick is 0.5ms. Multiply by two, shift by one, and
	// enable it.
	//
	if ( MaxTime_ms > WATCHDOG_MAX_TIME )
    {
        MaxTime_ms = WATCHDOG_MAX_TIME;   // max
    } /* if */
    sim.wrrr = (MaxTime_ms << 2) | 1;
    m_fEnabled = true;
    m_uTimeout = MaxTime_ms;
#endif

} /* Watchdog::Start() */


/********************************************************************
* Watchdog::Stop()
*
*   This function stops the CPU hardware watchdog timer.
*
* Parameters:
* 	void
*
* Return:
* 	void
*********************************************************************/
void Watchdog::Stop( void )
{
#ifdef SB72
    sim.wrrr &= 0xFFFE;
    m_fEnabled = false;
#endif

} /* Watchdog::Stop() */


/********************************************************************
* Watchdog::IsResetDueToWD()
*
*   This function reports whether the CPU was reset by the watchdog.
*
* Parameters:
* 	void
*
* Return:
*   TRUE if reset was due to watchdog, else FALSE
*
*********************************************************************/
bool Watchdog::IsResetDueToWD( void )
{
#ifdef SB72
	#define RSTSRC_WD	0x2000

    return (bool)(sim.scr & RSTSRC_WD);
#else
    return false;
#endif
} /* Watchdog::IsResetDueToWD() */


/********************************************************************
* Watchdog::Kick()
*
*   Calling this function kicks the hardware watchdog.  Register
*   WCR will be set to zero.
*
*   NOTE: When the WCN register reaches the WRR register value the
*         CPU will reset itself.
*
* Parameters:
* 	void
*
* Return:
* 	void
*
*********************************************************************/
void Watchdog::Kick( void )
{
#ifdef SB72
    sim.wcr = 0;
#endif
} /* Watchdog::Kick() */


/********************************************************************
* Watchdog::TestMode()
*   Putting the watchdog into test mode will allow all the normal
*   functions to work, except that the periodic kick will be disabled
*   allowing the watchdog to reset after the defined period
*
* Parameters:
* 	fTestMode - true to enable test mode
*
* Return:
*  	void
*********************************************************************/
void Watchdog::TestMode( bool fTestMode )
{
	m_fTestMode = fTestMode;

} /* Watchdog::TestMode() */



/********************************************************************
* Watchdog::TimeLeft()
*
*   This function will return the time left (in mSec), before a CPU
*   reset will occur (it is assumed that hardware watchdog is enabled
*   (WRRR regiser bit 0 = 1).
*
* Parameters:
* 	void
*
* Return:
*  	Time left in mSec before WDG will expire ( 0-16838 mSec )
*********************************************************************/
unsigned int Watchdog::TimeLeft( void )
{
#ifdef SB72
    return (((sim.wrrr >> 1) - (sim.wcr >> 1) ) >> 1 );
#else
    return (unsigned int)-1;
#endif
} /* Watchdog::TimeLeft() */


/********************************************************************
* Watchdog::TaskThreadWrapper()
*
* Parameters:
* 	pArgs - "this" pointer
*
* Return:
*  	void
*********************************************************************/
void Watchdog::TaskThreadWrapper( void* pArgs )
{
	// This is a static function, so we get the 'this' pointer from the args
	//
	Watchdog* pThis = (Watchdog*)pArgs;
	pThis->TaskThread();

} /* Watchdog::TaskThreadWrapper() */


/********************************************************************
* Watchdog::TaskThread()
*
* Parameters:
* 	void
*
* Return:
*  	void
*********************************************************************/
void Watchdog::TaskThread( void )
{
#ifdef SB72
	// Main thread loop, kick the watchdog every second.
	//
	m_fThreadRunning = true;
	while ( !m_fShutdownThread )
	{
		unsigned int uTimeLeft = TimeLeft();
		if ( uTimeLeft < m_uShortestTimeLeft )
		{
			m_uShortestTimeLeft = uTimeLeft;
			DebugTrace( 2, "Watchdog: shortest time left now %dms\r\n",
					    m_uShortestTimeLeft );
		} /* if */

		if ( m_fTestMode == false )
		{
			DebugTrace( 3, "Watchdog: kick!\r\n" );
			Kick();
		} /* if */
		OSTimeDly( TICKS_PER_SECOND );
	} /* while */
#endif

} /* Watchdog::TaskThread() */



int Watchdog::CmdWatchdog( int argc, char *argv[], void *pContext )
{
#ifdef SB72
	Watchdog* pThis = (Watchdog*)pContext;

	if ( argc == 1 )
	{
		Console.Printf( "\r\n" );
		Console.Printf( "Watchdog info\r\n" );
		Console.Printf( "            Enabled: %s\r\n",   pThis->m_fEnabled ? "yes" : "no" );
		Console.Printf( "            Timeout: %dms\r\n", pThis->m_uTimeout );
		Console.Printf( "         Last Reset: %s\r\n",   pThis->m_fLastResetDueToWD ? "watchdog" : "normal" );
		Console.Printf( " Shortest Time Left: %dms\r\n", pThis->m_uShortestTimeLeft );
		return 0;
	}

	if ( stricmp( argv[1], "enable" ) == 0 )
	{
		unsigned uTimeout = 5000;
		if ( argc >= 3 )
		{
			sscanf( argv[2], "%u", &uTimeout );
		}
		Console.Printf( "\r\nEnabling the watchdog with a %ums timeout\r\n", uTimeout );
		pThis->Start( uTimeout );
	}
	else if ( stricmp( argv[1], "disable" ) == 0 )
	{
		Console.Printf( "\r\nDisabling the watchdog\r\n" );
		pThis->Stop();
	}
	else if ( stricmp( argv[1], "test" ) == 0 )
	{
		Console.Printf( "\r\nTesting the watchdog\r\n" );
		pThis->TestMode( true );
	}
#else
	Console.Printf ("\r\nWatchdog not supported on this hardware\r\n" );
#endif
	return 0;
}


// The one and only instance of the watchdog class.
//
Watchdog g_Watchdog;


