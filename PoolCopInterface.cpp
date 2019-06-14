/*
 * PoolCopInterface.cpp
 *
 *  Created on: Jun 6, 2011
 *      Author: david
 */

#include <stdio.h>
#include <basictypes.h>
#include <command.h>
#include "PoolCopInterface.h"
#include "TaskPriorities.h"
#include "PoolCop.h"
#include "PoolCopDataBindings.h"
#include "ConfigData.h"
#include "Console.h"
#include "Utilities.h"
#include "DataWatchdog.h"
#include "PoolCopilot.h"


#define SIZEOF(x)						(sizeof(x)/sizeof(x[0]))
#define VALUE_IN_RANGE(val,min,max)		(val >= min && val <= max)

#define MSG_ACK				1
#define MSG_NAK				2

#define DebugTrace( level, fmt, ... )	Console.DebugTrace( MOD_POOLCOP, level, fmt, ##__VA_ARGS__ )

static DWORD 	g_PoolCopTaskStack[USER_TASK_STK_SIZE];



PoolCopInterface::PoolCopInterface() :
	m_iUart( 0 ),
	m_lBaudRate( 4800 ),
	m_iStopBits( 1 ),
	m_iDataBits( 8 ),
	m_eParity( eParityNone ),
	m_fdPoolCop( 0 ),
	m_fShutdownThread( false ),
	m_iBufIndx( 0 ),
	//m_fRefreshInProgress( false ),
	m_iValvePos( -1 )

{
	memset( m_RxBuffer, 0, sizeof(m_RxBuffer) );
	memset( m_TxBuffer, 0, sizeof(m_TxBuffer) );
	m_fTxEmpty = true;

} /* PoolCopInterface::PoolCopInterface() */


PoolCopInterface::~PoolCopInterface()
{
	// Signal thread to shutdown and wait for it to exit.
	//
	m_fShutdownThread = true;
	while ( m_fdPoolCop != 0 )
	{
		OSTimeDly( TICKS_PER_SECOND );
	}

} /* PoolCopInterface::~PoolCopInterface() */


int PoolCopInterface::Startup( void )
{
	int iRet;

	DebugTrace( 16, "%s: enter\n", __FUNCTION__ );

	OSMboxInit( &m_TxMbox, NULL );

	iRet = OSTaskCreate( TaskThreadWrapper,
						 (void*)this,
						 (void*)&g_PoolCopTaskStack[USER_TASK_STK_SIZE],
						 (void*)g_PoolCopTaskStack,
						 POOLCOP_SERIAL_PRIO );
	if ( iRet != OS_NO_ERR )
	{
		DebugTrace( 1, "%s: Error starting thread, err=%d\n", __FUNCTION__, iRet );
	}

	Console.AddCommand( "pc",      "Send message to PoolCop",          CmdSendMsg, this );
	Console.AddCommand( "valve",   "Rotate Poolcop valve",             CmdRotateValve, this );
	Console.AddCommand( "msgrate", "Set PoolCop message refresh rate", CmdMsgRate, this );

	return iRet;

} /* PoolCopInterface::Startup() */


int	PoolCopInterface::Shutdown( void )
{
	return 0;

} /* PoolCopInterface::Shutdown() */


void PoolCopInterface::Putchar( unsigned char cCh )
{
	if ( m_fdPoolCop != 0 )
	{
		write( m_fdPoolCop, (char*)&cCh, 1 );
	} /* if */

} /* PoolCopInterface::Putchar() */


int PoolCopInterface::Getchar( unsigned char *pcCh, int iTimeout )
{
	if ( m_fdPoolCop == 0 )
	{
		return -1;
	}

	int iRet = ReadWithTimeout( m_fdPoolCop, (char*)pcCh, 1, iTimeout );
	return iRet;

} /* PoolCopInterface::Getchar() */


void PoolCopInterface::TaskThreadWrapper( void* pArgs )
{
	// This is a static function, so we get the 'this' pointer from the args
	//
	PoolCopInterface* pThis = (PoolCopInterface*)pArgs;
	pThis->TaskThread();
}


void PoolCopInterface::TaskThread( void )
{
	// Open the serial port to the PoolCop
	//
	SerialClose( m_iUart );
	m_fdPoolCop = OpenSerial( m_iUart,
						      m_lBaudRate,
						      m_iStopBits,
							  m_iDataBits,
						      m_eParity );
    Serial485HalfDupMode( m_iUart, 0 );	// enable 485/422 on UART


	// Main thread loop, do handshaking, receive messages, transmit messages, etc.
	//
    int				iRet;
    unsigned char	cCh;
	while ( !m_fShutdownThread )
	{
		// Get a character with a 10th second timeout.
		//
		iRet = Getchar( &cCh, (TICKS_PER_SECOND / 10) );
		if ( iRet <= 0 )
		{
			// Check the TX queue for outgoing messages.
			//
			if ( m_fTxEmpty == false )
			{
				DebugTrace( 1, "\r\nGot TX message \"%s\"...", m_TxBuffer );
				char *p = m_TxBuffer;
				while ( *p != '\0' )
				{
					Putchar( *p++ );
				}
				Putchar( '\r' );

				m_TxBuffer[0] = '\0';
				m_fTxEmpty = true;
			}
			continue;
		} /* if */


		// At this point we have received a character - handle it accordingly.
		//
		if ( cCh == '\r' )
		{
			if ( m_iBufIndx > 0 )
			{
				m_RxBuffer[m_iBufIndx] = '\0';
				ParseBuffer();
				m_iBufIndx = 0;
				m_RxBuffer[0] = '\0';
			}
		}
		else
		{
			if ( cCh >= 0x20 && cCh <= 0x7F )
			{
				m_RxBuffer[m_iBufIndx++] = (char)cCh;
			} /* if */
		} /* else */

		int iErr = GetUartErrorReg( m_fdPoolCop );
		if ( iErr & 0x0001 )	DebugTrace( 14, "%s: OVERRUN\r\n", __FUNCTION__ );
		if ( iErr & 0x0002 )	DebugTrace( 14, "%s: PARITY\r\n",  __FUNCTION__ );
		if ( iErr & 0x0004 )	DebugTrace( 14, "%s: FRAMING\r\n", __FUNCTION__ );
		if ( iErr & 0x0008 )	DebugTrace( 14, "%s: BREAK\r\n",   __FUNCTION__ );

	} /* while */

	SerialClose( m_iUart );
	m_fdPoolCop = 0;

} /* PoolCopInterface::TaskThread() */


int PoolCopInterface::SendMessage( const char *pszMsg, bool fOverride )
{
	int iRet = ERROR_SUCCESS;

	DebugTrace( 13, "\r\nSending message to PoolCop: \"%s\"", pszMsg );

	// Wait for TX buffer to be free
	while ( m_fTxEmpty == false )
	{
		OSTimeDly( 2 );
	}

	// Add checksum to output message
	strcpy( m_TxBuffer, pszMsg );
#if 0
	strcat( m_TxBuffer, "*" );
#else
	AppendChecksum( m_TxBuffer );
#endif

    // Signal that message is in TX buffer
	m_fTxEmpty = false;

	// Wait for ACK/NAK response, or timeout.
	BYTE err;
	int iAckNak = (int)OSMboxPend( &m_TxMbox, 5*TICKS_PER_SECOND, &err );
	switch ( iAckNak )
	{
	case NULL:
		DebugTrace( 1, "\r\n%s - timeout", __FUNCTION__ );
		iRet = ERROR_TIMEOUT;
		break;
	case MSG_ACK:
		DebugTrace( 6, "\r\n%s - message ACKd", __FUNCTION__ );
		iRet = ERROR_SUCCESS;
		break;
	case MSG_NAK:
		DebugTrace( 1, "\r\n%s - message NAKd", __FUNCTION__ );
		iRet = ERROR_NAK_RCVD;
		break;
	default:
		DebugTrace( 1, "\r\n%s - unknown error", __FUNCTION__ );
		iRet = ERROR_OTHER;
		break;
    } /* switch */

	return iRet;
}


int PoolCopInterface::ParseBuffer( void )
{
	int     iRet = ERROR_SUCCESS;
	bool	fParsedMsg = false;

	DebugTrace( 16, "\r\n%s: enter", __FUNCTION__ );
	DebugTrace( 6, "\r\n%s: \"%s\"", __FUNCTION__, m_RxBuffer );


	if ( !IsChecksumValid( m_RxBuffer ) )
	{
		DebugTrace( 1, "\r\nInvalid checksum!" );
		return ERROR_BAD_CHECKSUM;
	}
	char *p = strrchr( m_RxBuffer, '*' );
	if ( p )
	{
		*p = '\0';
	}

	// Handle ACK/NAK response to each command sent to the PoolCop.
	//
	if ( strncmp( m_RxBuffer, "ACK", 3 ) == 0 )
	{
		OSMboxPost( &m_TxMbox, (void*)MSG_ACK );
		fParsedMsg = true;
	}
	else if ( strncmp( m_RxBuffer, "NAK", 3 ) == 0 )
	{
		OSMboxPost( &m_TxMbox, (void*)MSG_NAK );
		fParsedMsg = true;
	}

	// Handle "DG <x>" message - PoolCop Operational Status
	//
	else if ( strncmp( m_RxBuffer, "DG", 2 ) == 0 )
	{
		bool fHandled = true;

		g_PoolCopData.Lock();
		switch ( m_RxBuffer[3] )
		{
		case 'X':
			g_PoolCopData.fDataRateChanged = true;
			break;

		case 'Z':
			g_DataWatchdog.StaticDataRcvd();
			g_PoolCopData.fInitialDataRefresh = true;
            g_PoolCopData.fRefreshPending     = false;
			//m_fRefreshInProgress = false;
			break;

		default:
			fHandled = false;
			break;
		}
        g_PoolCopData.Unlock();

        if ( fHandled )
        {
        	fParsedMsg = true;
        }
	} /* if */


	// Handle DSA message - filter position, pressure, pH, water temperature and battery voltage.
	//
	else if ( strncmp( m_RxBuffer, "DSA", 3 ) == 0 )
	{
		int   iValvePos = 0, iOrp = 0;
		float fPressure = 0, fpH = 0, fBattVoltage = 0, fWaterTemp = 0;
		int	  iInput1State = 0;
		int   iInput2State = 0;
		int   iInput1Function = 0;
		int   iInput2Function = 0;
		int   iAirTemperature = 0;
		int   iMainsVoltage = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %f %f %f %f %d %d %d %d %d %d %d",
						   &iValvePos,
						   &fPressure,
						   &fpH,
						   &fBattVoltage,
						   &fWaterTemp,
						   &iOrp,
						   &iInput1State,
						   &iInput1Function,
						   &iInput2State,
						   &iInput2Function,
						   &iAirTemperature,
						   &iMainsVoltage);
		if ( iRet >= 10 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.dynamicData.ValvePosition    = iValvePos;
			g_PoolCopData.dynamicData.PumpPressure     = fPressure;
			g_PoolCopData.dynamicData.pH               = fpH;
			g_PoolCopData.dynamicData.BatteryVoltage   = fBattVoltage;
			g_PoolCopData.dynamicData.WaterTemperature = fWaterTemp;
			g_PoolCopData.dynamicData.Orp              = iOrp;
			g_PoolCopData.dynamicData.Input1State      = iInput1State;
			g_PoolCopData.dynamicData.Input1Function   = iInput1Function;
			g_PoolCopData.dynamicData.Input2State      = iInput2State;
			g_PoolCopData.dynamicData.Input2Function   = iInput2Function;
			g_PoolCopData.dynamicData.AirTemperature   = iAirTemperature;
			g_PoolCopData.dynamicData.MainsVoltage     = iMainsVoltage;
			g_PoolCopData.dynamicData.NewData          = true;
			g_PoolCopData.Unlock();

			if ( iValvePos != m_iValvePos )
			{
				m_iValvePos = iValvePos;
				g_PoolCopilot.AsyncDataRcvd();
			} /* if */

			fParsedMsg = true;
			g_DataWatchdog.DynamicDataRcvd();
		}
	} /* if */


	// Handle DSB message - ORP dynamic data
	//
	else if ( strncmp( m_RxBuffer, "DSB", 3 ) == 0 )
	{
		int     iInstalled = 0;
		int		iSetPoint = 0;
		int 	iDisinfectantType = 0;
		int		iOrpControl = 0;
		int		iLastInjection = 0;
		int	    iHyperchlorationSetPoint = 0;
		int		iHyperchlorationWeekday = 0;
		int 	iLowShutdownTemp = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d",
						   &iInstalled,
						   &iSetPoint,
						   &iDisinfectantType,
						   &iOrpControl,
						   &iLastInjection,
						   &iHyperchlorationSetPoint,
						   &iHyperchlorationWeekday,
						   &iLowShutdownTemp);
		if ( iRet >= 7 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.orpControl.Installed               = (bool)iInstalled;
			g_PoolCopData.orpControl.SetPoint                = iSetPoint;
			g_PoolCopData.orpControl.DisinfectantType        = iDisinfectantType;
			g_PoolCopData.orpControl.OrpControl              = iOrpControl;
			g_PoolCopData.orpControl.LastInjection           = iLastInjection;
			g_PoolCopData.orpControl.HyperchlorationSetPoint = iHyperchlorationSetPoint;
			g_PoolCopData.orpControl.HyperchlorationWeekday  = iHyperchlorationWeekday;
			g_PoolCopData.orpControl.LowShutdownTemp         = iLowShutdownTemp;		// PoolCop v30+
			g_PoolCopData.orpControl.NewData                 = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSC message - relay settings (pump, aux1-7, valve pos)
	//
	else if ( strncmp( m_RxBuffer, "DSC", 3 ) == 0 )
	{
		int iPumpStatus = 0, iValvePos = 0;
		int iAux[AUX_COUNT] = { 0 };
		int ipHStatus;
		int	iPoolCopStatus = 0;
		int iPumpCurrentSpeed = 0;
		int	iPumpForcedHrsRemaining = 0;
		int iRunningStatus = 0;
		int iValveRotationsCounter = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				           &iPumpStatus,
				           &iAux[0],
				           &iAux[1],
				           &iAux[2],
				           &iAux[3],
				           &iAux[4],
				           &iAux[5],
		 		           &ipHStatus,
				           &iValvePos,
				           &iPoolCopStatus,
				           &iPumpCurrentSpeed,
				           &iPumpForcedHrsRemaining,
				           &iRunningStatus,
				           &iValveRotationsCounter);
		if ( iRet >= 13 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.dynamicData.PumpState     = iPumpStatus;
			for ( int i = 0; i < AUX_COUNT; i++ )
			{
				if ( iAux[i] != g_PoolCopData.auxSettings.Aux[i].Status )
				{
					g_PoolCopData.auxSettings.Aux[i].Status  = iAux[i];
					g_PoolCopData.auxSettings.Aux[i].NewData = true;
				}
			}
			if ( ipHStatus != g_PoolCopData.pHControl.Status )
			{
				g_PoolCopData.pHControl.Status  = ipHStatus;
				g_PoolCopData.pHControl.NewData = true;
			}
			g_PoolCopData.dynamicData.ValvePosition          = iValvePos;
			g_PoolCopData.dynamicData.PoolCopStatus          = iPoolCopStatus;
			g_PoolCopData.dynamicData.PumpCurrentSpeed       = iPumpCurrentSpeed;
			g_PoolCopData.dynamicData.PumpForcedHrsRemaining = iPumpForcedHrsRemaining;
		    g_PoolCopData.dynamicData.RunningStatus          = iRunningStatus;
		    g_PoolCopData.dynamicData.ValveRotationsCounter  = iValveRotationsCounter;
			g_PoolCopData.dynamicData.NewData                = true;
			g_PoolCopData.auxSettings.NewData                = true;
			g_PoolCopData.Unlock();

			if ( iPoolCopStatus >= 1 && iPoolCopStatus <= 4 )
			{
				m_fPoolCopBusy = true;
			}
			else
			{
				m_fPoolCopBusy = false;
			}

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSD message - date
	//
	else if ( strncmp( m_RxBuffer, "DSD", 3 ) == 0 )
	{
		int iYear = 0, iMonth = 0, iDay = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d",
						   &iMonth,
						   &iDay,
						   &iYear );
		if ( iRet == 3 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.dynamicData.Date.Year  = iYear;
			g_PoolCopData.dynamicData.Date.Month = iMonth;
			g_PoolCopData.dynamicData.Date.Day   = iDay;
			g_PoolCopData.dynamicData.NewData    = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSE message - Ionizer/Chlorinator info
	//
	else if ( strncmp( m_RxBuffer, "DSE", 3 ) == 0 )
	{
		int iIonizerInstalled = 0,
		    iIonizerMode = 0,
		    iIonizerStatus = 0,
		    iChlorinatorInstalled = 0,
		    iChlorinatorMode = 0,
		    iChlorinatorStatus = 0,
		    iAcidStatus = 0,
		    iIonizerDuration = 0,
		    iGasDuration = 0,
		    iIonizerCurrent = 0,
		    iIonizerCurrentMode = 0,
		    iIonizerLastInjection = 0,
		    iGasLastInjection = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d %d %d %d %d",
			               &iIonizerInstalled,
			               &iIonizerMode,
			               &iIonizerStatus,
			               &iChlorinatorInstalled,
			               &iChlorinatorMode,
			               &iChlorinatorStatus,
			               &iAcidStatus,
			               &iIonizerDuration,
			               &iGasDuration,
			               &iIonizerCurrent,
			               &iIonizerCurrentMode,
			               &iIonizerLastInjection,
			               &iGasLastInjection );
		if ( iRet == 13 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.ioniser.Installed     = (bool)iIonizerInstalled;
			g_PoolCopData.ioniser.Mode          = iIonizerMode;
			g_PoolCopData.ioniser.Status        = iIonizerStatus;
			g_PoolCopData.ioniser.Duration      = iIonizerDuration;
			g_PoolCopData.ioniser.Current       = iIonizerCurrent;
			g_PoolCopData.ioniser.CurrentMode   = iIonizerCurrentMode;
			g_PoolCopData.ioniser.LastInjection = iIonizerLastInjection;
			g_PoolCopData.ioniser.NewData       = true;

			g_PoolCopData.autoChlor.Installed        = (bool)iChlorinatorInstalled;
			g_PoolCopData.autoChlor.Mode             = iChlorinatorMode;
			g_PoolCopData.autoChlor.Status           = iChlorinatorStatus;
			g_PoolCopData.autoChlor.AcidStatus       = iAcidStatus;
			g_PoolCopData.autoChlor.GasDuration      = iGasDuration;
			g_PoolCopData.autoChlor.GasLastInjection = iGasLastInjection;
			g_PoolCopData.autoChlor.NewData          = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSF message - TideMaster info
	//
	else if ( strncmp( m_RxBuffer, "DSF", 3 ) == 0 )
	{
		int iInstalled = 0,
		    iPumpStatus = 0,
		    iWaterValve = 0,
		    iWaterLevel = 0,
		    iAutoWaterAdd = 0,
		    iCableStatus = 0,
		    iReserved = 0,
		    iContinuousFill = 0,
		    iMaxFillDuration = 0,
		    iAutoWaterReduce = 0,
		    iDrainingDuration = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d %d %d",
			               &iInstalled,
			               &iPumpStatus,
			               &iWaterValve,
			               &iWaterLevel,
			               &iAutoWaterAdd,
			               &iCableStatus,
			               &iReserved,
			               &iContinuousFill,
			               &iMaxFillDuration,
			               &iAutoWaterReduce,
			               &iDrainingDuration);
		if ( iRet >= 9 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.waterLevelControl.Installed        = (bool)iInstalled;
			g_PoolCopData.waterLevelControl.PumpStatus       = iPumpStatus;
			g_PoolCopData.waterLevelControl.WaterValve       = iWaterValve;
			g_PoolCopData.waterLevelControl.WaterLevel       = iWaterLevel;
			g_PoolCopData.waterLevelControl.AutoWaterAdd     = (bool)iAutoWaterAdd;
			g_PoolCopData.waterLevelControl.CableStatus      = iCableStatus;
		    g_PoolCopData.waterLevelControl.ContinuousFill   = (bool)iContinuousFill;
			g_PoolCopData.waterLevelControl.MaxFillDuration  = iMaxFillDuration;
			g_PoolCopData.waterLevelControl.AutoWaterReduce  = iAutoWaterReduce;
			g_PoolCopData.waterLevelControl.DrainingDuration = iDrainingDuration;
			g_PoolCopData.waterLevelControl.NewData          = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSG message - Pump Data
	//
	else if ( strncmp( m_RxBuffer, "DSG", 3 ) == 0 )
	{
		int iPressureLow = 0,
			iAlarmPressure = 0,
			iPumpProtect = 0,
			iPumpType = 0,
			iSpeedCycle1 = 0,
			iSpeedCycle2 = 0,
			iSpeedBackwash = 0,
			iNumSpeeds = 0,
			iForcedPumpMode = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d",
						   &iPressureLow,
						   &iAlarmPressure,
						   &iPumpProtect,
						   &iPumpType,
						   &iSpeedCycle1,
						   &iSpeedCycle2,
						   &iSpeedBackwash,
						   &iNumSpeeds,
						   &iForcedPumpMode );
		if ( iRet == 9 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.pumpSettings.PressureLow    = iPressureLow;
			g_PoolCopData.pumpSettings.AlarmPressure  = iAlarmPressure;
			g_PoolCopData.pumpSettings.PumpProtect    = (bool)iPumpProtect;
			g_PoolCopData.pumpSettings.PumpType       = iPumpType;
			g_PoolCopData.pumpSettings.SpeedCycle1    = iSpeedCycle1;
			g_PoolCopData.pumpSettings.SpeedCycle2    = iSpeedCycle2;
			g_PoolCopData.pumpSettings.SpeedBackwash  = iSpeedBackwash;
			g_PoolCopData.pumpSettings.NumSpeeds      = iNumSpeeds;
			g_PoolCopData.pumpSettings.ForcedPumpMode = iForcedPumpMode;
			g_PoolCopData.pumpSettings.NewData	      = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	}


	// Handle DSH message - pH Control info
	//
	else if ( strncmp( m_RxBuffer, "DSH", 3 ) == 0 )
	{
		int	iInstalled = 0,
		    iSetPoint = 0,
		    ipHType = 0,
		    iDosingTime = 0,
		    iLastInjection = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d",
				           &iInstalled,
				           &iSetPoint,
				           &ipHType,
				           &iDosingTime,
				           &iLastInjection );
		if ( iRet == 5 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.pHControl.Installed      = (bool)iInstalled;
			g_PoolCopData.pHControl.SetPoint       = (float)iSetPoint / 10;
			g_PoolCopData.pHControl.pHType         = ipHType;
			g_PoolCopData.pHControl.DosingTime     = iDosingTime;
			g_PoolCopData.pHControl.LastInjection  = iLastInjection;
			g_PoolCopData.pHControl.NewData        = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSI message - Poolcop version
	//
	else if ( strncmp( m_RxBuffer, "DSI", 3 ) == 0 )
	{
		char	zVersion[80] = { 0 };

		strncpy( zVersion, &m_RxBuffer[4], sizeof(zVersion)-1 );
		if ( zVersion[0] != '\0' )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.configData.PoolCopVersion = zVersion;
			g_PoolCopData.configData.NewData        = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	}


	// Handle DSJ message - Factory setting restored
	//
	else if ( strncmp( m_RxBuffer, "DSJ", 3 ) == 0 )
	{
		// TODO: Handle this message properly
	} /* if */


	// Handle DSK message - filter backwash settings
	//
	else if ( strncmp( m_RxBuffer, "DSK", 3 ) == 0 )
	{
		/* DSK ppp bbb rrr a d with
		 *     ppp: High Presure setting to launch backwash
		 *     bbb: backwash duration from 10sec to 600 sec (10sec steps)
		 *     rrr: rinse duration from 10sec to 180sec (10sec steps)
		 *       a: automatic backwash on high pressure ("no"/"yes")
		 *       d: Weekly backwash day  from 0 to 7 (0=none, 1=Monday, 2=Tuesday...7=Sunday)
		 */
		int iPressure               = 0,
		    iBackwashDuration       = 0,
		    iRinseDuration          = 0,
		    iAutoBackwash           = 0,
		    iMaxDaysBetweenBackwash = 0,
		    iWasteLineValve         = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%03d %03d %03d %1d %d %d",
				           &iPressure,
				           &iBackwashDuration,
				           &iRinseDuration,
				           &iAutoBackwash,
				           &iMaxDaysBetweenBackwash,
				           &iWasteLineValve);
		if ( iRet >= 5 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.filterSettings.Pressure               = iPressure;
			g_PoolCopData.filterSettings.BackwashDuration       = iBackwashDuration;
			g_PoolCopData.filterSettings.RinseDuration          = iRinseDuration;
			g_PoolCopData.filterSettings.AutoBackwash           = iAutoBackwash;
			g_PoolCopData.filterSettings.MaxDaysBetweenBackwash = iMaxDaysBetweenBackwash;
			g_PoolCopData.filterSettings.WasteLineValve         = iWasteLineValve;
			g_PoolCopData.filterSettings.NewData                = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSS message
	//
	else if ( strncmp( m_RxBuffer, "DSS", 3 ) == 0 )
	{
		int iNumBackwashes     = 0,
			iLastBackwashYear  = 0,
			iLastBackwashMonth = 0,
			iLastBackwashDay   = 0,
			iLastBackwashHour  = 0,
			iLastBackwashMin   = 0,
			iNumRefills        = 0,
			iLastRefillYear    = 0,
			iLastRefillMonth   = 0,
			iLastRefillDay     = 0,
			iLastRefillHour    = 0,
			iLastRefillMin     = 0,
			iLastPhYear        = 0,
			iLastPhMonth       = 0,
			iLastPhDay         = 0,
			iLastPhHour        = 0,
			iLastPhMin         = 0;


		int iRet = sscanf( &m_RxBuffer[4],
				           "%4d %04d %2d %2d %2d %2d %4d %04d %2d %2d %2d %2d %4d %2d %2d %2d %2d",
				           &iNumBackwashes,
				           &iLastBackwashYear,
				           &iLastBackwashMonth,
				           &iLastBackwashDay,
				           &iLastBackwashHour,
				           &iLastBackwashMin,
				           &iNumRefills,
				           &iLastRefillYear,
						   &iLastRefillMonth,
						   &iLastRefillDay,
						   &iLastRefillHour,
						   &iLastRefillMin,
						   &iLastPhYear,
						   &iLastPhMonth,
						   &iLastPhDay,
						   &iLastPhHour,
						   &iLastPhMin );
		if ( iRet == 17 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.historyData.NumBackwashes           = iNumBackwashes;
			g_PoolCopData.historyData.LastBackwashDate.Year   = iLastBackwashYear;
			g_PoolCopData.historyData.LastBackwashDate.Month  = iLastBackwashMonth;
			g_PoolCopData.historyData.LastBackwashDate.Day    = iLastBackwashDay;
			g_PoolCopData.historyData.LastBackwashTime.Hour   = iLastBackwashHour;
			g_PoolCopData.historyData.LastBackwashTime.Minute = iLastBackwashMin;
			g_PoolCopData.historyData.NumRefills              = iNumRefills;
			g_PoolCopData.historyData.LastRefillDate.Year     = iLastRefillYear;
			g_PoolCopData.historyData.LastRefillDate.Month    = iLastRefillMonth;
			g_PoolCopData.historyData.LastRefillDate.Day      = iLastRefillDay;
			g_PoolCopData.historyData.LastRefillTime.Hour     = iLastRefillHour;
			g_PoolCopData.historyData.LastRefillTime.Minute   = iLastRefillMin;
			g_PoolCopData.historyData.LastPhDate.Year         = iLastPhYear;
			g_PoolCopData.historyData.LastPhDate.Month        = iLastPhMonth;
			g_PoolCopData.historyData.LastPhDate.Day          = iLastPhDay;
			g_PoolCopData.historyData.LastPhTime.Hour         = iLastPhHour;
			g_PoolCopData.historyData.LastPhTime.Minute       = iLastPhMin;
			g_PoolCopData.historyData.NewData                 = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSM message - Inputs
	else if ( strncmp( m_RxBuffer, "DSM", 3 ) == 0 )
	{
		int 	iInput1Dir  = 0;
		int 	iInput1Func = 0;
		int 	iInput2Dir  = 0;
		int 	iInput2Func = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d",
						   &iInput1Dir,
						   &iInput1Func,
						   &iInput2Dir,
						   &iInput2Func );
		if ( iRet == 4 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.inputs.Input1Dir  = iInput1Dir;
			g_PoolCopData.inputs.Input1Func = iInput1Func;
			g_PoolCopData.inputs.Input2Dir  = iInput2Dir;
			g_PoolCopData.inputs.Input2Func = iInput2Func;
			g_PoolCopData.inputs.NewData    = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	}

	// Handle DSO message - Alarms
	//
	else if ( strncmp( m_RxBuffer, "DSO", 3 ) == 0 )
	{
		int iAlarms[ALARM_COUNT] = { 0 };

		// Parse each of the alarms, up to ALARM_COUNT in all.
		int  indx   = 0;
		char *str   = strdup( &m_RxBuffer[4] );
		char *token = strtok( str, " " );
		while ((token != NULL) && (indx < ALARM_COUNT))
		{
			iAlarms[indx++] = atoi( token );
			token = strtok( NULL, " " );
		}
		free( str );

		// Copy the alarms to the global data.
		g_PoolCopData.Lock();
		g_PoolCopData.dynamicData.NumAlarms = indx;
		for ( int i = 0; i < indx; i++ )
		{
		   	if ( iAlarms[i] != g_PoolCopData.dynamicData.Alarms[i] )
		   	{
		   		g_PoolCopData.dynamicData.NewData = true;
		   	}
		   	g_PoolCopData.dynamicData.Alarms[i] = iAlarms[i];
		}
		g_PoolCopData.Unlock();
		fParsedMsg = true;
	}


	// Handle DSP message - Pool config settings
	//
	else if ( strncmp( m_RxBuffer, "DSP", 3 ) == 0 )
	{
		int	iVolume = 0,
		    iFlowRate = 0,
		    iTurnovers = 0,
		    iFreezeProtection = 0,
		    iCoverReductionPct = 0,
		    iPoolType = 0,
		    iCoverSpeed = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d",
	  			           &iVolume,
	  			           &iFlowRate,
	  			           &iTurnovers,
	  			           &iFreezeProtection,
	  			           &iCoverReductionPct,
	  			           &iPoolType,
	  			           &iCoverSpeed);
		if ( iRet >= 5 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.poolSettings.PoolVolume        = iVolume;
			g_PoolCopData.poolSettings.FlowRate          = iFlowRate;
			g_PoolCopData.poolSettings.TurnoversPerDay   = iTurnovers;
			g_PoolCopData.poolSettings.FreezeProtection  = iFreezeProtection;
			g_PoolCopData.poolSettings.CoverReductionPct = iCoverReductionPct;
			g_PoolCopData.poolSettings.PoolType          = iPoolType;
			g_PoolCopData.poolSettings.CoverSpeed        = iCoverSpeed;
			g_PoolCopData.poolSettings.NewData           = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* else if */


	// Handle DSR message - Service Mode - PoolCop v30+ only
	else if ( strncmp( m_RxBuffer, "DSR", 3 ) == 0 )
	{
		int iServiceMode = 0;
		int iAirAntifreezeTrigger = 0;
		int iWaterTempShift = 0;
		int ipHCalibration = 0;
		int ipHCalibrationStatus = 0;


		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d",
						   &iServiceMode,
						   &iAirAntifreezeTrigger,
						   &iWaterTempShift,
						   &ipHCalibration,
						   &ipHCalibrationStatus );
		if ( iRet >= 1 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.poolSettings.ServiceMode          = iServiceMode;
			g_PoolCopData.poolSettings.AirAntifreezeTrigger = iAirAntifreezeTrigger;
			g_PoolCopData.poolSettings.WaterTempShift       = iWaterTempShift;
			g_PoolCopData.poolSettings.pHCalibration        = ipHCalibration;
			g_PoolCopData.poolSettings.pHCalibrationStatus  = ipHCalibrationStatus;
			g_PoolCopData.poolSettings.NewData              = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* else if */


	// Handle DST message - Time
	//
	else if ( strncmp( m_RxBuffer, "DST", 3 ) == 0 )
	{
		int iHour = 0,
		    iMinute = 0,
		    iSecond = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d",
				           &iHour,
				           &iMinute,
				           &iSecond );
		if ( iRet ==  3 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.dynamicData.Time.Hour   = iHour;
			g_PoolCopData.dynamicData.Time.Minute = iMinute;
			g_PoolCopData.dynamicData.Time.Second = iSecond;
			g_PoolCopData.dynamicData.NewData     = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSV message - Aux Timers
	//
	else if ( strncmp( m_RxBuffer, "DSV", 3 ) == 0 )
	{
		int	iAuxNbr = 0,
		    iSlave = 0,
		    iStatus = 0,
		    iStartHour = 0,
		    iStartMin = 0,
		    iStopHour = 0,
		    iStopMin = 0,
		    iRunsMonday = 0,
		    iRunsTuesday = 0,
		    iRunsWednesday = 0,
		    iRunsThursday = 0,
		    iRunsFriday = 0,
		    iRunsSaturday = 0,
		    iRunsSunday = 0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
						   &iAuxNbr,
						   &iSlave,
						   &iStatus,
						   &iStartHour,
						   &iStartMin,
						   &iStopHour,
						   &iStopMin,
						   &iRunsMonday,
						   &iRunsTuesday,
						   &iRunsWednesday,
						   &iRunsThursday,
						   &iRunsFriday,
						   &iRunsSaturday,
						   &iRunsSunday );
		if ( iRet == 14 )
		{
			if ( iAuxNbr >= 1 && iAuxNbr <= AUX_COUNT )
			{
				g_PoolCopData.Lock();
				iAuxNbr -= 1;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].Slave          = (bool)iSlave;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].TimerStatus    = iStatus;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].TimeOn.Hour    = iStartHour;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].TimeOn.Minute  = iStartMin;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].TimeOff.Hour   = iStopHour;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].TimeOff.Minute = iStopMin;

				char zTmp[8] = "0000000";
				sprintf( zTmp, "%1d%1d%1d%1d%1d%1d%1d",
						 iRunsMonday,
						 iRunsTuesday,
						 iRunsWednesday,
						 iRunsThursday,
						 iRunsFriday,
						 iRunsSaturday,
						 iRunsSunday );
				g_PoolCopData.auxSettings.Aux[iAuxNbr].DaysOfWeek = zTmp;
				g_PoolCopData.auxSettings.Aux[iAuxNbr].NewData    = true;
				g_PoolCopData.auxSettings.NewData = true;
				g_PoolCopData.Unlock();
			} /* if */

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSW message - Pump Timer Info
	//
	else if ( strncmp( m_RxBuffer, "DSW", 3 ) == 0 )
	{
		int	iFlag = 0,
		    iMode = 0,
		    iStartHour1 = 0,
		    iStartMin1 = 0,
		    iStopHour1 = 0,
		    iStopMin1 = 0,
		    iStartHour2 = 0,
		    iStartMin2 = 0,
		    iStopHour2 = 0,
		    iStopMin2 = 0;
		float fMinTemp = 0.0,
		      fMaxTemp = 0.0;

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d %d %d %d %d %f %f",
				           &iFlag,
				           &iMode,
				           &iStartHour1,
				           &iStartMin1,
				           &iStopHour1,
				           &iStopMin1,
				           &iStartHour2,
				           &iStartMin2,
				           &iStopHour2,
				           &iStopMin2,
				           &fMaxTemp,
				           &fMinTemp);
		if ( iRet == 12 )
		{
			g_PoolCopData.Lock();
			g_PoolCopData.filterSettings.TimerMode                  = iMode;

			g_PoolCopData.filterSettings.TimerCycle1.Enabled        = (bool)(iFlag & 0x01);
			g_PoolCopData.filterSettings.TimerCycle1.TimeOn.Hour    = iStartHour1;
			g_PoolCopData.filterSettings.TimerCycle1.TimeOn.Minute  = iStartMin1;
			g_PoolCopData.filterSettings.TimerCycle1.TimeOff.Hour   = iStopHour1;
			g_PoolCopData.filterSettings.TimerCycle1.TimeOff.Minute = iStopMin1;

			g_PoolCopData.filterSettings.TimerCycle2.Enabled        = (bool)(iFlag & 0x02);
			g_PoolCopData.filterSettings.TimerCycle2.TimeOn.Hour    = iStartHour2;
			g_PoolCopData.filterSettings.TimerCycle2.TimeOn.Minute  = iStartMin2;
			g_PoolCopData.filterSettings.TimerCycle2.TimeOff.Hour   = iStopHour2;
			g_PoolCopData.filterSettings.TimerCycle2.TimeOff.Minute = iStopMin2;

			g_PoolCopData.filterSettings.MaxTemp = fMaxTemp;
			g_PoolCopData.filterSettings.MinTemp = fMinTemp;

			g_PoolCopData.filterSettings.NewData                    = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DSX message - Aux Labels
	//
	else if ( strncmp( m_RxBuffer, "DSX", 3 ) == 0 )
	{
		int iAuxLabels[AUX_COUNT] = { 0 };

		int iRet = sscanf( &m_RxBuffer[4], "%d %d %d %d %d %d",
				           &iAuxLabels[0],
				           &iAuxLabels[1],
				           &iAuxLabels[2],
				           &iAuxLabels[3],
				           &iAuxLabels[4],
				           &iAuxLabels[5] );
		if ( iRet == 6 )
		{
			g_PoolCopData.Lock();
			for ( int i = 0; i < AUX_COUNT; i++ )
			{
				if ( g_PoolCopData.auxSettings.Aux[i].Label != iAuxLabels[i] )
				{
					g_PoolCopData.auxSettings.Aux[i].Label   = iAuxLabels[i];
					g_PoolCopData.auxSettings.Aux[i].NewData = true;
				}
			}
			g_PoolCopData.auxSettings.NewData = true;
			g_PoolCopData.Unlock();

			fParsedMsg = true;
		}
	} /* if */


	// Handle DXY message - Mains supply failed
	//
	else if ( strncmp( m_RxBuffer, "DXY", 3 ) == 0 )
	{
		g_PoolCopData.Lock();
		g_PoolCopData.dynamicData.MainsPowerLost = true;
		g_PoolCopData.dynamicData.NewData        = true;
		g_PoolCopData.Unlock();

		fParsedMsg = true;
	}


	// Handle DXY message - Mains supply normal
	//
	else if ( strncmp( m_RxBuffer, "DXZ", 3 ) == 0 )
	{
		g_PoolCopData.Lock();
		g_PoolCopData.dynamicData.MainsPowerLost = false;
		g_PoolCopData.dynamicData.NewData        = true;
		g_PoolCopData.Unlock();

		fParsedMsg = true;
	}


	// Display debug message for any unhandled messages.
	//
	if ( !fParsedMsg )
	{
		DebugTrace( 2, "%s: unhandled msg \"%s\"\n", __FUNCTION__, m_RxBuffer );
		iRet = 1;
	} /* if */
	else
	{
		g_PoolCopData.Lock();
		strncpy( g_PoolCopData.zLastRxMsg, m_RxBuffer, sizeof(g_PoolCopData.zLastRxMsg)-1 );
		g_PoolCopData.dwLastRxTime = Secs;	// Number of seconds since boot
		g_PoolCopData.Unlock();
	} /* else */

	DebugTrace( 16, "%s: leave, ret=%d\n", __FUNCTION__, iRet );
	return iRet;

} /* PoolCopInterface::ParseBuffer() */



// Send "CMN x"/"CMF x" message to PoolCop to control auxiliary
//
int	PoolCopInterface::ControlAux( const int iAux,
		                          const int iState )
{
	char	szMsg[20];

	if ( !VALUE_IN_RANGE(iAux,1,8) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CM%c %d", iState ? 'N':'F', iAux );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CMN 7"/"CMF 7" message to PoolCop to control pump
//
int PoolCopInterface::ControlPump( const int iState )
{
	int iRet = ControlAux( 7, iState );
	return iRet;
}


// Send "CMN 8"/"CMF 8" message to PoolCop to water level add
//
int	PoolCopInterface::WaterLevelAdd( const int iState )
{
	int iRet = ControlAux( 8, iState );
	return iRet;
}


// Send "CMB" message to PoolCop to command it to backwash the filter
//
int PoolCopInterface::BackwashFilter( void )
{
	char 	szMsg[20] = "CMB";
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CMR x" message to PoolCop to command it to rotate valve
//
int PoolCopInterface::RotateValve( const VALVEPOS ePos )
{
	char 	szMsg[20];

	if ( !VALUE_IN_RANGE(ePos,0,5) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CMR %1d", (int)ePos );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CMP" message to PoolCop to command it to measure pH
//
int PoolCopInterface::MeasurePh( void )
{
	char 	szMsg[20] = "CMP";
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CMH" message to PoolCop to command it to calibrate pH
//
int PoolCopInterface::CalibratePh( int iCalibrationValue )
{
	char 	szMsg[20];

	if ( !VALUE_IN_RANGE(iCalibrationValue,65,80) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
     	return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CMH %02d", iCalibrationValue );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CML" message to PoolCop to select language
//
int PoolCopInterface::SelectLanguage( char *language )
{
	char 	szMsg[20];

	sprintf( szMsg, "CML %s", language );
	int iRet = SendMessage( szMsg );
	return iRet;
}



// Send "CMX xxx" message to PoolCop to control message refresh rate
//
int PoolCopInterface::MessageUpdateRate( const int iRate )
{
	char 	szMsg[20];

	if ( !VALUE_IN_RANGE(iRate,0,999) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CMX %03d", iRate );
	int iRet = SendMessage( szMsg, true );
	return iRet;
}


// Send "RSM" message to PoolCop to set input configuration
//
int PoolCopInterface::SetInputConfig( const int iInput1Dir,
		                              const int iInput1Func,
		                              const int iInput2Dir,
		                              const int iInput2Func )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iInput1Dir,0,1) ||
	     !VALUE_IN_RANGE(iInput1Func,0,9) ||
	     !VALUE_IN_RANGE(iInput2Dir,0,1) ||
	     !VALUE_IN_RANGE(iInput2Func,0,9) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSM %1d %1d %1d %1d",
			 iInput1Dir,
			 iInput1Func,
			 iInput2Dir,
			 iInput2Func );
	int iRet = SendMessage( szMsg, true );
	return iRet;
}


// Send "RSB" message to PoolCop to set ORP configuration
//
int PoolCopInterface::SetOrpConfig( const int iInstalled,
		                            const int iSetPoint,
		                            const int iType,
		                            const int iHyperchlorationSetPoint,
					                const int iHyperchlorationWeekday,
					                const int iLowShutdownTemp )
{
	char 	szMsg[40];

	if ( !VALUE_IN_RANGE(iInstalled,0,1) ||
		 !VALUE_IN_RANGE(iSetPoint,0,999) ||
	     !VALUE_IN_RANGE(iType,0,5) ||
	     !VALUE_IN_RANGE(iHyperchlorationSetPoint,0,999) ||
	     !VALUE_IN_RANGE(iHyperchlorationWeekday,0,7) ||
	     !VALUE_IN_RANGE(iLowShutdownTemp,0,18) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSB %1d %03d %1d %03d %1d %02d",
			 iInstalled,
			 iSetPoint,
			 iType,
			 iHyperchlorationSetPoint,
			 iHyperchlorationWeekday,
			 iLowShutdownTemp );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSD" message to PoolCop to set date
//
int PoolCopInterface::SetDate( const char *pzDate )
{
	int		iRet = 0;
	char	szMsg[80];

	if ( pzDate != NULL )
	{
		int iYear, iMonth, iDay;
		sscanf( pzDate, "%04d/%02d/%02d",
				&iYear,
				&iMonth,
				&iDay );

		if ( !VALUE_IN_RANGE(iYear,2000,2200) ||
		     !VALUE_IN_RANGE(iMonth,1,12) ||
			 !VALUE_IN_RANGE(iDay,1,31) )
		{
			DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
			return ERROR_INVALID_RANGE;
		}

		sprintf( szMsg, "RSD %02d %02d %04d",
				 iMonth,
				 iDay,
				 iYear );
		iRet = SendMessage( szMsg );
	}
	return iRet;
}


// Send "RST" message to PoolCop to set date
//
int PoolCopInterface::SetTime( const char *pzTime )
{
	int		iRet = 0;
	char	szMsg[80];

	if ( pzTime != NULL )
	{
		int iHour, iMinute, iSecond;
		sscanf( pzTime, "%02d:%02d:%02d",
				&iHour,
				&iMinute,
				&iSecond );

		if ( !VALUE_IN_RANGE(iHour,0,23) ||
			 !VALUE_IN_RANGE(iMinute,0,59) ||
			 !VALUE_IN_RANGE(iSecond,0,59) )
		{
			DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
			return ERROR_INVALID_RANGE;
		}

		sprintf( szMsg, "RST %02d %02d %02d",
				 iHour,
				 iMinute,
				 iSecond );
		iRet = SendMessage( szMsg );
	}
	return iRet;
}


// Send "RSE" message to set Ionizer configuration
//
int PoolCopInterface::SetIonizerConfig( int iInstalled,
		                                int iMode,
		                                int iMaxPct,
		                                int iCurrentInPctMode )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iInstalled,0,1) ||
		 !VALUE_IN_RANGE(iMode,0,1)      ||
		 !VALUE_IN_RANGE(iMaxPct,10,100) ||
		 !VALUE_IN_RANGE(iCurrentInPctMode,0,2) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSE %1d %1d %03d %1d",
			 iInstalled,
			 iMode,
			 iMaxPct,
			 iCurrentInPctMode );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSL" message to set Autochlor configuration
//
int PoolCopInterface::SetAutochlorConfig( const int iGasMode,
		                                  const int iGasDuration,
                                          const int iSetpoint )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iGasMode,0,1) ||
		 !VALUE_IN_RANGE(iGasDuration,0,100) ||
		 !VALUE_IN_RANGE(iSetpoint,65,80) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSL %1d %03d %02d",
			 iGasMode,
			 iGasDuration,
             iSetpoint );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSF" message to set Water Level configuration
//
int PoolCopInterface::SetWaterLevelControl( int iInstalled,
		                                    int iAutoAdd,
		                                    int iAddContinuous,
		                                    int iMaxDuration,
		                                    int iAutoWaterReduce,
		                                    int iDrainingDuration)
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iInstalled,0,1) ||
		 !VALUE_IN_RANGE(iAutoAdd,0,1) ||
		 !VALUE_IN_RANGE(iAddContinuous,0,1) ||
		 !VALUE_IN_RANGE(iMaxDuration,0,720) ||
		 !VALUE_IN_RANGE(iAutoWaterReduce,0,1) ||
		 !VALUE_IN_RANGE(iDrainingDuration,10,600) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSF %1d %1d %1d %03d %d %03d",
			 iInstalled,
			 iAutoAdd,
			 iAddContinuous,
			 iMaxDuration,
			 iAutoWaterReduce,
			 iDrainingDuration );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSG" message to set Pump configuration
//
int	PoolCopInterface::SetPumpConfig( const int iPressureLow,
	                            	 const int iAlarmPressure,
	                            	 const int iPumpProtect,
	                            	 const int iPumpType,
	                            	 const int iSpeedCycle1,
	                            	 const int iSpeedCycle2,
	                            	 const int iSpeedBackwash )
{
	char 	szMsg[40];

	if ( !VALUE_IN_RANGE(iPressureLow,0,200) ||
		 !VALUE_IN_RANGE(iAlarmPressure,5,100) ||
		 !VALUE_IN_RANGE(iPumpProtect,0,1) ||
		 !VALUE_IN_RANGE(iPumpType,0,20) ||
		 !VALUE_IN_RANGE(iSpeedCycle1,0,8) ||
		 !VALUE_IN_RANGE(iSpeedCycle2,0,8) ||
		 !VALUE_IN_RANGE(iSpeedBackwash,0,7) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSG %03d %03d %1d %1d %1d %1d %1d",
             iPressureLow,
             iAlarmPressure,
			 iPumpProtect,
			 iPumpType,
			 iSpeedCycle1,
			 iSpeedCycle2,
			 iSpeedBackwash );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSH" message to set pH configuration
//
int PoolCopInterface::SetPhConfig( const int   iInstalled,
			                       const float fSetPoint,
			                       const int   iType,
			                       const int   iMaxDosingTime )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iInstalled,0,1) ||
		 !VALUE_IN_RANGE(fSetPoint,6.5,8.0) ||
		 !VALUE_IN_RANGE(iType,0,2) ||
		 !VALUE_IN_RANGE(iMaxDosingTime,1,30) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSH %1d %02d %1d %02d",
			 iInstalled,
			 (int)(fSetPoint * 10),
			 iType,
			 iMaxDosingTime );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSI" message to set clear alarm
//
int PoolCopInterface::ClearAlarm( const int iAlarm )
{
	char 	szMsg[80];

	if ( !VALUE_IN_RANGE(iAlarm,1,ALARM_COUNT) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSI %02d", iAlarm );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSJ" message to reset PoolCop to factory defaults
//
int PoolCopInterface::RestoreFactorySettings( const char *pzConfirm )
{
	int iRet = SendMessage( "RSJ" );
	return iRet;
}


// Send "RSK" message to set PoolCop filter parameters
//
int PoolCopInterface::SetFilterParameters( int iPressure,
										   int iBackwashDuration,
										   int iRinseDuration,
										   int iAutoBackwash,
										   int iMaxDaysBetweenBackwash,
										   int iWasteLineValve)
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iPressure,0,200) ||
		 !VALUE_IN_RANGE(iBackwashDuration,10,600) ||
		 !VALUE_IN_RANGE(iRinseDuration,10,180) ||
		 !VALUE_IN_RANGE(iAutoBackwash,0,2) ||
		 !VALUE_IN_RANGE(iMaxDaysBetweenBackwash,0,250) ||
		 !VALUE_IN_RANGE(iWasteLineValve,0,1) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSK %03d %03d %03d %1d %03d %d",
			 iPressure,
			 iBackwashDuration,
			 iRinseDuration,
			 iAutoBackwash,
			 iMaxDaysBetweenBackwash,
			 iWasteLineValve);
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSP" message to set Pool configuration
//
int PoolCopInterface::SetPoolConfig( const int iVolume,
						             const int iFlowRate,
						             const int iTurnovers,
						             const int iFreezeProtection,
						             const int iCoverReductionPct,
						             const int iPoolType,
						             const int iCoverSpeed)
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iVolume,1,999) ||
		 !VALUE_IN_RANGE(iFlowRate,1,99) ||
		 !VALUE_IN_RANGE(iTurnovers,1,99) ||
		 !VALUE_IN_RANGE(iFreezeProtection,0,1) ||
		 !VALUE_IN_RANGE(iCoverReductionPct,0,70) ||
		 !VALUE_IN_RANGE(iPoolType,0,2) ||
		 !VALUE_IN_RANGE(iCoverSpeed,0,8))
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSP %03d %02d %02d %1d %02d %1d %1d",
	  	     iVolume,
			 iFlowRate,
			 iTurnovers,
			 iFreezeProtection,
			 iCoverReductionPct,
			 iPoolType,
			 iCoverSpeed);
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSR" message to set service mode - PoolCop v30+ only
//
int PoolCopInterface::SetServiceMode( const int iServiceMode,
		                              const int iAirAntifreezeTrigger,
		                              const int iWaterTempShift )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iServiceMode,0,2) ||
	     !VALUE_IN_RANGE(iAirAntifreezeTrigger,0,20) ||
	     !VALUE_IN_RANGE(iWaterTempShift,0,200) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSR %d %d %d", iServiceMode,
			                        iAirAntifreezeTrigger,
			                        iWaterTempShift );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSV" message to set auxiliary timers
//
int PoolCopInterface::SetAuxTimer( const int  iAux,
						           const int  iTimerSlave,
						           const int  iTimerStatus,
						           const char *pzStartTime,
						           const char *pzStopTime,
						           const int  iRunMonday,
						           const int  iRunTuesday,
						           const int  iRunWednesday,
						           const int  iRunThursday,
						           const int  iRunFriday,
						           const int  iRunSaturday,
						           const int  iRunSunday )
{
	int iStartHour = 0,
	    iStartMin  = 0,
	    iStopHour  = 0,
	    iStopMin   = 0;
	if ( pzStartTime )
    {
        sscanf( pzStartTime, "%02d:%02d", &iStartHour, &iStartMin );
    }
    if ( pzStopTime )
    {
        sscanf( pzStopTime, "%02d:%02d", &iStopHour,  &iStopMin );
    }

	if ( !VALUE_IN_RANGE(iAux,1,6) ||
		 !VALUE_IN_RANGE(iTimerSlave,0,1) ||
		 !VALUE_IN_RANGE(iTimerStatus,0,1) ||
		 !VALUE_IN_RANGE(iStartHour,0,23) ||
		 !VALUE_IN_RANGE(iStartMin,0,59) ||
		 !VALUE_IN_RANGE(iStopHour,0,23) ||
		 !VALUE_IN_RANGE(iStopMin,0,59) ||
		 !VALUE_IN_RANGE(iRunMonday,0,1) ||
		 !VALUE_IN_RANGE(iRunTuesday,0,1) ||
		 !VALUE_IN_RANGE(iRunWednesday,0,1) ||
		 !VALUE_IN_RANGE(iRunThursday,0,1) ||
		 !VALUE_IN_RANGE(iRunFriday,0,1) ||
		 !VALUE_IN_RANGE(iRunSaturday,0,1) ||
		 !VALUE_IN_RANGE(iRunSunday,0,1) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	char 	szMsg[30];
	sprintf( szMsg, "RSV %1d %1d %1d %02d %02d %02d %02d %1d %1d %1d %1d %1d %1d %1d",
		     iAux,
		     iTimerSlave,
		     iTimerStatus,
		     iStartHour,
		     iStartMin,
		     iStopHour,
		     iStopMin,
		     iRunMonday,
		     iRunTuesday,
		     iRunWednesday,
		     iRunThursday,
		     iRunFriday,
		     iRunSaturday,
		     iRunSunday );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSW" message to set filter timers
//
int PoolCopInterface::SetFilterTimers( const int  iTimer1Enable,
		                               const int  iTimer2Enable,
									   const int  iTimer2Mode,
									   const char *pzStartTime1,
									   const char *pzStopTime1,
									   const char *pzStartTime2,
									   const char *pzStopTime2 )
{
	int		iRet;
	char	szMsg[80];


	int iTimerFlag = 0;
	if ( iTimer1Enable )
    {
        iTimerFlag |= 0x01;
    }
    if ( iTimer2Enable )
    {
        iTimerFlag |= 0x02;
    }

	int iStartHr1  = 0,
	    iStartMin1 = 0,
	    iStopHr1   = 0,
	    iStopMin1  = 0;
	if ( pzStartTime1 )
    {
        sscanf( pzStartTime1, "%02d:%02d", &iStartHr1, &iStartMin1 );
    }
    if ( pzStopTime1)
    {
        sscanf( pzStopTime1,  "%02d:%02d", &iStopHr1,  &iStopMin1 );
    }

	int iStartHr2  = 0,
	    iStartMin2 = 0,
	    iStopHr2   = 0,
	    iStopMin2  = 0;
	if ( pzStartTime2 )
    {
        sscanf( pzStartTime2, "%02d:%02d", &iStartHr2, &iStartMin2 );
    }
    if ( pzStopTime2)
    {
        sscanf( pzStopTime2,  "%02d:%02d", &iStopHr2,  &iStopMin2 );
    }

    if ( !VALUE_IN_RANGE(iTimerFlag,0,3) ||
    	 !VALUE_IN_RANGE(iTimer2Mode,0,9) ||		// PoolCop v30+ allows 0-9, else 0-2
    	 !VALUE_IN_RANGE(iStartHr1,0,23) ||
    	 !VALUE_IN_RANGE(iStartMin1,0,59) ||
    	 !VALUE_IN_RANGE(iStopHr1,0,23) ||
    	 !VALUE_IN_RANGE(iStopMin1,0,59) ||
    	 !VALUE_IN_RANGE(iStartHr2,0,23) ||
 	     !VALUE_IN_RANGE(iStartMin2,0,59) ||
    	 !VALUE_IN_RANGE(iStopHr2,0,23) ||
    	 !VALUE_IN_RANGE(iStopMin2,0,59) )
    {
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSW %1d %1d %02d %02d %02d %02d %02d %02d %02d %02d",
			 iTimerFlag,
			 iTimer2Mode,
			 iStartHr1, iStartMin1,
			 iStopHr1,  iStopMin1,
			 iStartHr2, iStartMin2,
			 iStopHr2,  iStopMin2 );
	iRet = SendMessage( szMsg );

	return iRet;
}


// Send "RSX" message to set auxiliary labels
//
#define AUX_LABEL_MAX		99
int PoolCopInterface::SetAuxLabels( const int iAux1,
		                            const int iAux2,
		                            const int iAux3,
		                            const int iAux4,
		                            const int iAux5,
		                            const int iAux6 )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iAux1,0,AUX_LABEL_MAX) ||
		 !VALUE_IN_RANGE(iAux2,0,AUX_LABEL_MAX) ||
		 !VALUE_IN_RANGE(iAux3,0,AUX_LABEL_MAX) ||
		 !VALUE_IN_RANGE(iAux4,0,AUX_LABEL_MAX) ||
		 !VALUE_IN_RANGE(iAux5,0,AUX_LABEL_MAX) ||
		 !VALUE_IN_RANGE(iAux6,0,AUX_LABEL_MAX) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSX %02d %02d %02d %02d %02d %02d",
		     iAux1,
		     iAux2,
		     iAux3,
		     iAux4,
		     iAux5,
		     iAux6 );
	int iRet = SendMessage( szMsg );
	return iRet;
}


int PoolCopInterface::RequestData( const int iInfoType,
		                           const int iAux )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iInfoType,1,26) ||
		 !VALUE_IN_RANGE(iAux,1,6) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSY %02d %01d",
		     iInfoType,
		     iAux );
	int iRet = SendMessage( szMsg );
	return iRet;
}



// Send "CMS" message to set pump speed
//
int PoolCopInterface::SetPumpSpeed( const int iPumpSpeed )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iPumpSpeed,0,8) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CMS %1d", iPumpSpeed );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "CMZ" message to reset the PoolCop
//
int PoolCopInterface::ResetPoolCop( void )
{
	int iRet = SendMessage( "CMZ" );
	return iRet;
}


// Send "CMI" message to calibrate pH
//
int PoolCopInterface::pHAutoCalibration( const int iCalibrationValue )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iCalibrationValue,0,200) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "CMI %d", iCalibrationValue );
	int iRet = SendMessage( szMsg );
	return iRet;
}



// Send "RSC" message to set pump mode
//
int PoolCopInterface::SetForcedPumpMode( const int iPumpMode )
{
	char 	szMsg[30];

	if ( !VALUE_IN_RANGE(iPumpMode,0,7) )
	{
		DebugTrace( 1, "\r\n%s: Invalid parameter(s)", __FUNCTION__ );
		return ERROR_INVALID_RANGE;
	}

	sprintf( szMsg, "RSC %1d", iPumpMode );
	int iRet = SendMessage( szMsg );
	return iRet;
}


// Send "RSZ" message to command PoolCop to resend all parameters and settings
//
int PoolCopInterface::RefreshData( REFRESH_TYPE eType )
{
	int iRet;

	if ( eType == REFRESH_STATIC )
	{
		g_PoolCopData.Lock();
		g_PoolCopData.fRefreshPending = true;
		g_PoolCopData.Unlock();
		iRet = SendMessage( "RSZ", true );
		//m_fRefreshInProgress = true;
	}
	else
	{
		char	szMsg[20];
		sprintf( szMsg, "RSZ %d", (int)eType );
		iRet = SendMessage( szMsg, true );
	}
	return iRet;
}


int PoolCopInterface::CmdSendMsg( int argc, char *argv[], void *pContext )
{
	PoolCopInterface *pThis = (PoolCopInterface *)pContext;

	char szMsg[100] = { 0 };
	for ( int i = 1; i < argc; i++ )
	{
		strcat( szMsg, argv[i] );
		strcat( szMsg, " " );
	}
	pThis->SendMessage( szMsg );
	return 0;

}


int PoolCopInterface::CmdRotateValve( int argc, char *argv[], void *pContext )
{
	PoolCopInterface *pThis = (PoolCopInterface *)pContext;
	int iRet = 0;

	if ( argc <= 1 )
	{
		Console.Printf( "\r\nUsage: rotate filter|waste|closed|backwash|bypass|rinse" );
		return -1;
	}

	if ( stricmp( argv[1], "filter" ) == 0 )
		iRet = pThis->RotateValve( VALVE_FILTER );
	else if ( stricmp( argv[1], "waste" ) == 0 )
		iRet = pThis->RotateValve( VALVE_WASTE );
	else if ( stricmp( argv[1], "closed" ) == 0 )
		iRet = pThis->RotateValve( VALVE_CLOSED );
	else if ( stricmp( argv[1], "backwash" ) == 0 )
		iRet = pThis->RotateValve( VALVE_BACKWASH );
	else if ( stricmp( argv[1], "bypass" ) == 0 )
		iRet = pThis->RotateValve( VALVE_BYPASS );
	else if ( stricmp( argv[1], "rinse" ) == 0 )
		iRet = pThis->RotateValve( VALVE_RINSE );
	else
	{
		Console.Printf( "\r\nUnrecognized parameter '%s'", argv[1] );
		iRet = -1;
	}
	return iRet;
}

int PoolCopInterface::CmdMsgRate( int argc, char *argv[], void *pContext )
{
	PoolCopInterface *pThis = (PoolCopInterface *)pContext;
	int iRet = 0;

	if ( argc <= 1 )
	{
		Console.Printf( "\r\nUsage: msgrate [0-999]" );
		return -1;
	}

	int iRate = atoi( argv[1] );
	iRet = pThis->MessageUpdateRate( iRate );
	return iRet;
}
