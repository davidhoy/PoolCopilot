/*
 * PoolCopInterface.h
 *
 *  Created on: Jun 6, 2011
 *      Author: david
 */

#ifndef POOLCOPINTERFACE_H_
#define POOLCOPINTERFACE_H_

#include <stdio.h>
#include <startnet.h>
#include <serial.h>
#include <iosys.h>
#include <string.h>
#include <ucos.h>


#define BUFFER_SIZE			120

#define ERROR_SUCCESS		 0
#define ERROR_TIMEOUT		-1
#define ERROR_NAK_RCVD		-2
#define ERROR_INVALID_RANGE	-3
#define ERROR_BAD_CHECKSUM	-4
#define ERROR_OTHER			-5


typedef enum
{
	VALVE_FILTER = 0,
	VALVE_WASTE,
	VALVE_CLOSED,
	VALVE_BACKWASH,
	VALVE_BYPASS,
	VALVE_RINSE
} VALVEPOS;

typedef enum
{
	REFRESH_STATIC = 1,
	REFRESH_DYNAMIC = 2,
	REFRESH_BOTH = 3
} REFRESH_TYPE;


class PoolCopInterface
{

public:
	PoolCopInterface();
	~PoolCopInterface();

	int			Startup( void );
	int			Shutdown( void );

	void 		Putchar( unsigned char cCh );
	int 		Getchar( unsigned char *pcCh, int iTimeout );
	int			Getchar( void );

	static void TaskThreadWrapper( void* pArgs );
	void        TaskThread( void );
	int			ParseBuffer( void );

	bool		IsBusy( void )				{ return m_fPoolCopBusy; }

    int			ControlPump( const int iState );

    int			ControlAux( const int iAux,
    		                const int iState );

    int			WaterLevelAdd( const int iState );

    int			BackwashFilter( void );

    int			RotateValve( const VALVEPOS ePos );

    int         SetTime( const char *pzTime );

	int 		MeasurePh( void );

	int         CalibratePh( int iCalibrationValue );

	int 		SelectLanguage( char *language );

	int 		MessageUpdateRate( const int iRate );

	int 		SetOrpConfig( const int iInstalled,
			                  const int iSetPoint,
			                  const int iType,
			                  const int iHyperchlorationSetPoint,
			                  const int iHyperchlorationWeekday,
			                  const int iLowShutdownTemp );

	int         SetDate( const char *pzDate );

	int 		SetIonizerConfig( const int iInstalled,
			                      const int iMode,
			                      const int iMaxPct,
			                      const int iCurrentInPctMode );

	int         SetAutochlorConfig( const int iGasMode,
			                        const int iGasDuration,
                                    const int iSetpoint );

	int 		SetWaterLevelControl( const int iInstalled,
			                          const int iAutoAdd,
			                          const int iAddContinuous,
			                          const int iMaxDuration,
			                          const int iAutoWaterReduce,
			                          const int iDrainingDuration);

	int			SetPumpConfig( const int iPressureLow,
							   const int iAlarmPressure,
							   const int iPumpProtect,
							   const int iPumpType,
							   const int iSpeedCycle1,
							   const int iSpeedCycle2,
							   const int iSpeedBackwash );

	int 		SetPhConfig( const int   iInstalled,
			                 const float fSetPoint,
			                 const int   iType,
			                 const int   iMaxDosingTime );

	int			ClearAlarm( const int iAlarm );

	int 		SetFilterParameters( const int iPressure,
			   	   	   	   	   	     const int iBackwashDuration,
			   	   	   	   	   	     const int iRinseDuration,
			   	   	   	   	   	     const int iAutoBackwash,
			   	   	   	   	   	     const int iMaxDaysBetweenBackwash,
			   	   	   	   	   	     const int iWasteLineValve);

	int			RestoreFactorySettings( const char *pzConfirm );

	int 		SetPoolConfig( const int iVolume,
							   const int iFlowRate,
							   const int iTurnovers,
							   const int iFreezeProtection,
							   const int iCoverReductionPct,
							   const int iPoolType,
							   const int iCoverSpeed);

	int         SetServiceMode( const int iServiceMode,
                                const int iAirAntifreezeTrigger,
                                const int iWaterTempShift );

	int 		SetAuxTimer( const int  iAux,
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
			                 const int  iRunSunday );

	int			SetFilterTimers( const int  iTimer1Enable,
			                     const int  iTimer2Enable,
			                     const int  iTimer2Mode,
			                     const char *pzStartTime1,
			                     const char *pzStopTime1,
			                     const char *pzStartTime2,
			                     const char *pzStopTime2 );

	int         SetAuxLabels( const int iAux1,
			                  const int iAux2,
			                  const int iAux3,
			                  const int iAux4,
			                  const int iAux5,
			                  const int iAux6 );

	int 		RequestData( const int iInfoType,
			                 const int iAux );

	int			SetPumpSpeed( const int iPumpSpeed );

	int 		SetForcedPumpMode( const int iPumpMode );

	int 		SetInputConfig( const int iInput1Dir,
			                    const int iInput1Func,
			                    const int iInput2Dir,
			                    const int iInput2Func );

	int         pHAutoCalibration( const int iCalibrationValue );

    int         RefreshData( REFRESH_TYPE eType = REFRESH_STATIC );

    int         ResetPoolCop( void );


	static int  CmdSendMsg( int argc, char *argv[], void *pContext );
	static int  CmdRotateValve( int argc, char *argv[], void *pContext );
	static int  CmdMsgRate( int argc, char *argv[], void *pContext );


private:
	int			SendMessage( const char *pszMsg, bool fOverride = false );
	int			m_iUart;
	long		m_lBaudRate;
	int 		m_iStopBits;
	int			m_iDataBits;
	parity_mode	m_eParity;
	int			m_fdPoolCop;

	bool		m_fShutdownThread;

	char		m_RxBuffer[BUFFER_SIZE];
	int			m_iBufIndx;
	//bool		m_fRefreshInProgress;

	char        m_LastRxMsg[BUFFER_SIZE];
	DWORD       m_LastRxTime;

	bool		m_fTxEmpty;
	char		m_TxBuffer[BUFFER_SIZE];
	OS_MBOX		m_TxMbox;

	bool		m_fPoolCopBusy;

	int			m_iValvePos;
};


extern PoolCopInterface g_PoolCop;

#endif /* POOLCOPINTERFACE_H_ */

