/*
 * DataWatchdog.cpp
 *
 *  Created on: Jan 29th, 2012
 *      Author: david
 */

#include <stdio.h>
#include <basictypes.h>
//#include <string.h>
#include "ucos.h"
#include "TaskPriorities.h"
#include "DataWatchdog.h"
#include "Console.h"
#include "PoolCopInterface.h"


#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_MAIN, level, fmt, ##__VA_ARGS__ )

static DWORD 	g_DataWatchdogTaskStack[USER_TASK_STK_SIZE];


/********************************************************************
* DataWatchdog::DataWatchog()
*
* Parameters:   void
*
* Return:       N/A
*********************************************************************/
DataWatchdog::DataWatchdog() :
	m_fThreadRunning      ( false ),
	m_fShutdownThread     ( false ),
	m_dwLastStaticData    (  0 ),
	m_dwStaticDataTimeout ( 30 ),
	m_dwLastDynamicData   (  0 ),
	m_dwDynamicDataTimeout( 30 )
{
}


/********************************************************************
* DataWatchdog::~DataWatchdog()
*
* Parameters:   void
*
* Return:       N/A
*********************************************************************/
DataWatchdog::~DataWatchdog()
{
}


/********************************************************************
* DataWatchdog::Start()
*
* Parameters:   void
*
* Return:       void
*********************************************************************/
void DataWatchdog::Start( void )
{
	// Create a normal-priority thread to monitor incoming data
	// from PoolCop
	//
	if ( m_fThreadRunning == false )
	{
		int iRet = OSTaskCreate( TaskThreadWrapper,
								 (void*)this,
								 (void*)&g_DataWatchdogTaskStack[USER_TASK_STK_SIZE],
								 (void*)g_DataWatchdogTaskStack,
								 DATAWATCHDOG_PRIO );
		if ( iRet != OS_NO_ERR )
		{
			DebugTrace( 1, "%s: Error starting thread, err=%d\n", __FUNCTION__, iRet );
		} /* if */
	} /* if */

} /* DataWatchdog::Start() */


/********************************************************************
* DataWatchdog::TaskThreadWrapper()
*
* Parameters:   pArgs - "this" pointer
*
* Return:       void
*********************************************************************/
void DataWatchdog::TaskThreadWrapper( void* pArgs )
{
	// This is a static function, so we get the 'this' pointer from the args
	//
	DataWatchdog* pThis = (DataWatchdog*)pArgs;
	pThis->TaskThread();

} /* DataWatchdog::TaskThreadWrapper() */


/********************************************************************
* DataWatchdog::TaskThread()
*
* Parameters:   void
*
* Return:       void
*********************************************************************/
void DataWatchdog::TaskThread( void )
{
	// Task thread loop
	//
	m_fThreadRunning = true;
	while ( !m_fShutdownThread )
	{
		OSTimeDly( 10 * TICKS_PER_SECOND );

		DWORD dwNow = TimeTick;

		// If we have not received static data in 30 seconds, send an "RSZ"
		// message to PoolCop.
		DWORD dwStaticElapsed = (dwNow - GetStaticDataRcvd()) / TICKS_PER_SECOND;
		if ( dwStaticElapsed > m_dwStaticDataTimeout )
		{
			DebugTrace( 2, "\r\n%s: no static data in %d sec, sending refresh",
					    __FUNCTION__, m_dwStaticDataTimeout );
			g_PoolCop.RefreshData( REFRESH_STATIC );
		}

		// If we have not received dynamic data in 30 seconds, send a "CMX <n>"
		// message to PoolCop.
		DWORD dwDynamicElapsed = (dwNow - GetDynamicDataRcvd()) / TICKS_PER_SECOND;
		if ( dwDynamicElapsed > m_dwDynamicDataTimeout )
		{
			DebugTrace( 2, "\r\n%s: no dynamic data in %d sec, resetting msg rate",
					    __FUNCTION__, m_dwDynamicDataTimeout );
			g_PoolCop.RefreshData( REFRESH_DYNAMIC );
			g_PoolCop.MessageUpdateRate( 1 );
		}

	} /* while */

} /* DataWatchdog::TaskThread() */


// The one and only instance of the DataWatchdog class.
//
DataWatchdog g_DataWatchdog;


