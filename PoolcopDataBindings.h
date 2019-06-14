

#ifndef _POOLCOPDATABINDING_H_
#define _POOLCOPDATABINDING_H_

#include <string>
#include "CritSec.h"


#define AUX_COUNT	6
#define ALARM_COUNT	50


struct XmlDate
{
    int     Year;
    int     Month;
    int     Day;

    XmlDate() {}
    XmlDate( int year, int month, int day )
    {
        Year = year; Month = month; Day = day;
    }
    XmlDate( const char *str )
    {
        sscanf( str, "%04d-%02d-%02d",
                &Year, &Month, &Day );
    }
};

struct XmlTime
{
    int     Hour;
    int     Minute;
    int     Second;

    XmlTime() {}
    XmlTime( int hour, int minute, int second )
    {
        Hour = hour; Minute = minute; Second = second;
    }
    XmlTime( const char *str )
    {
        sscanf( str, "%02d:%02d:%02d",
                &Hour, &Minute, &Second );
    }
};


struct NetworkConfiguration
{
    bool        UseDHCP;
    std::string	IPAddress;
    std::string	NetMask;
    std::string	DNSAddress;
    std::string	GatewayAddress;
    std::string	MACAddress;
    bool	    NewData;

    NetworkConfiguration() :
		UseDHCP( false ),
		IPAddress( "" ),
		NetMask( "" ),
		DNSAddress( "" ),
		GatewayAddress( "" ),
		MACAddress( "" ),
		NewData( false )
	{
	}
};


struct PoolCopilotConfiguration
{
    std::string	ServerURL;
    std::string AltServerURL;
    int         ConnectionInterval;
    bool        AutoUpdate;
    std::string	AutoUpdateURL;
    int         AutoUpdateFrequency;
    int         BaudRate;
    std::string	TimeServer;
    int         ResetCounter;
    bool     	NewData;

    PoolCopilotConfiguration() :
		ServerURL( "" ),
		ConnectionInterval( 300 ),
		AutoUpdate( false ),
		AutoUpdateURL( "" ),
		AutoUpdateFrequency( 0 ),
		BaudRate( 0 ),
		TimeServer( "" ),
        ResetCounter( 0 ),
		NewData( false )
	{
	}
};


struct DynamicData
{
	int			PoolCopStatus;
	int			ValvePosition;
    int         PumpState;
    float       PumpPressure;
    float       WaterTemperature;
    float       BatteryVoltage;
    float       pH;
    int         Orp;
    int 		Input1State;
    int  		Input2State;
    int         Input1Function;
    int			Input2Function;
    int      	MainsPowerLost;
    int         NumAlarms;
    int			Alarms[ALARM_COUNT];
    XmlDate     Date;
    XmlTime     Time;
    int			PumpCurrentSpeed;
    int			PumpForcedHrsRemaining;
    int			RunningStatus;
    int         AirTemperature;
    int         ValveRotationsCounter;
    int         MainsVoltage;
    bool   		NewData;

    DynamicData() :
        PoolCopStatus( 0 ),
    	ValvePosition( 0 ),
		PumpState( 0 ),
		PumpPressure( 0 ),
		WaterTemperature( 0 ),
		BatteryVoltage( 0 ),
		pH( 0 ),
		Orp( 0 ),
		Input1State( 0 ),
		Input2State( 0 ),
		MainsPowerLost( 0 ),
		NumAlarms( 0 ),
		Date( "0000-00-00" ),
		Time( "00:00:00" ),
		PumpCurrentSpeed( 0 ),
		PumpForcedHrsRemaining( 0 ),
		RunningStatus( 0 ),
		AirTemperature( 0 ),
		ValveRotationsCounter( 0 ),
		MainsVoltage( 0 ),
		NewData( false )
	{
    	for ( int i = 0; i < ALARM_COUNT; i++ )
    		Alarms[i] = 0;
	}
};


struct PoolSettings
{
    int     PoolVolume;
    int     FlowRate;
    int     TurnoversPerDay;
    int		FreezeProtection;
    int     CoverReductionPct;
    int     PoolType;
    int     CoverSpeed;
    int     ServiceMode;
    int     AirAntifreezeTrigger;
    int     WaterTempShift;
    int     pHCalibration;
    int     pHCalibrationStatus;
    bool   	NewData;

    PoolSettings() :
		PoolVolume( 0 ),
		FlowRate( 0 ),
		TurnoversPerDay( 0 ),
		CoverReductionPct( 0 ),
		PoolType( 0 ),
		CoverSpeed( 0 ),
		ServiceMode( 0 ),
		AirAntifreezeTrigger( 0 ),
		WaterTempShift( 0 ),
		pHCalibration( 0 ),
		pHCalibrationStatus( 0 ),
		NewData( false )
	{
	}
};


struct ConfigurationData
{
    NetworkConfiguration        networkConfig;
    PoolCopilotConfiguration    poolcopilotConfig;
    std::string					PoolCopVersion;
    std::string					PoolCopilotVersion;
	std::string                 HardwareType;
    bool   						NewData;


    ConfigurationData() :
		PoolCopVersion( "0.0" ),
		PoolCopilotVersion( "0.0.0" ),
#ifdef SB72
		HardwareType( "SB72" ),
#else
		HardwareType( "SB700EX" ),
#endif
		NewData( false )
	{

	}
};


struct TimerCycle
{
    bool 		Enabled;
    XmlTime     TimeOn;
    XmlTime     TimeOff;

    TimerCycle() :
		Enabled( false ),
		TimeOn( "00:00:00" ),
		TimeOff( "00:00:00" )
	{
	}
};


struct FilterSettings
{
    int 	    Pressure;
    int         BackwashDuration;
    int         RinseDuration;
    int 		AutoBackwash;
    int 		MaxDaysBetweenBackwash;
    int         WasteLineValve;
    int			TimerMode;
    TimerCycle  TimerCycle1;
    TimerCycle  TimerCycle2;
    float	    MaxTemp;
    float	    MinTemp;
    bool   		NewData;

    FilterSettings() :
		Pressure( 0 ),
		BackwashDuration ( 0 ),
		RinseDuration( 0 ),
		AutoBackwash( 0 ),
		MaxDaysBetweenBackwash( 0 ),
		WasteLineValve( 0 ),
      //AutoWeekly( 0 ),
		TimerMode( 0 ),
		NewData( false )
	{
	}
};


struct PumpSettings
{
    int         PressureLow;
    int			AlarmPressure;
    bool        PumpProtect;
    int			PumpType;
    int			SpeedCycle1;
    int			SpeedCycle2;
    int			SpeedBackwash;
    int			NumSpeeds;
    int         ForcedPumpMode;
    bool   		NewData;

    PumpSettings() :
		PressureLow ( 0 ),
		AlarmPressure( 0 ),
		PumpProtect( false ),
		PumpType( 0 ),
		SpeedCycle1( 0 ),
		SpeedCycle2( 0 ),
		SpeedBackwash( 0 ),
		NumSpeeds( 0 ),
		NewData( false )
	{
	}
};


struct Auxiliary
{
	int			Label;
    bool        Slave;
    int         Status;
    int         TimerStatus;
    XmlTime     TimeOn;
    XmlTime     TimeOff;
    std::string DaysOfWeek;
    bool   		NewData;

    Auxiliary() :
    	Label( 0 ),
		Slave( false ),
		Status( 0 ),
		TimeOn( "00:00:00" ),
		TimeOff( "00:00:00" ),
	    DaysOfWeek( "0000000" ),
	    NewData( false )
	{
	}
};


struct Auxiliaries
{
	Auxiliary	Aux[AUX_COUNT];
    bool       	NewData;

    Auxiliaries() :
    	NewData( false )
    {
    }
};

struct Inputs
{
	int 	Input1Dir;
	int 	Input1Func;
	int 	Input2Dir;
	int 	Input2Func;
	bool	NewData;

	Inputs() :
		Input1Dir( 0 ),
		Input1Func( 0 ),
		Input2Dir( 0 ),
		Input2Func( 0 ),
		NewData( false )
	{}
};


struct WaterLevelControl
{
	bool	    Installed;
    int		    PumpStatus;
    int  		WaterValve;
    int			WaterLevel;
    bool 		AutoWaterAdd;
    int 		CableStatus;
    bool 		ContinuousFill;
    int			MaxFillDuration;
    int         AutoWaterReduce;
    int 	    DrainingDuration;
    bool   		NewData;

    WaterLevelControl() :
		Installed( false ),
		PumpStatus( 0 ),
		WaterValve( 0 ),
		WaterLevel( 0 ),
		AutoWaterAdd( false ),
		CableStatus( 0 ),
		ContinuousFill( false ),
		MaxFillDuration( 0 ),
		AutoWaterReduce( 0 ),
		DrainingDuration( 0 ),
		NewData( false )
	{
    }
};


struct PhControl
{
    bool        Installed;
    bool 		Status;
    float       SetPoint;
    int			pHType;
    int         DosingTime;
    int			LastInjection;
    bool   		NewData;

    PhControl() :
		Installed( false ),
		Status( false ),
		SetPoint( 0.0 ),
		pHType( 0 ),
		DosingTime( 0 ),
		LastInjection( 0 ),
		NewData( false )
	{
	}
};


struct ORPControl
{
    bool        Installed;
    int         SetPoint;
    int			DisinfectantType;
    int			OrpControl;
    int			LastInjection;
    int			HyperchlorationSetPoint;
    int			HyperchlorationWeekday;
    int         LowShutdownTemp;
    bool   		NewData;

    ORPControl() :
		Installed( false ),
		SetPoint( 0 ),
		DisinfectantType( 0 ),
		OrpControl( 0 ),
		LastInjection( 0 ),
		HyperchlorationSetPoint( 0 ),
		HyperchlorationWeekday( 0 ),
		LowShutdownTemp( 0 ),				// PoolCop v30+
		NewData( false )
	{
    }
};


struct Ioniser
{
    bool        Installed;
    int  		Mode;
    int			Status;
    int         Duration;
    int			Current;
    int			CurrentMode;
    int			LastInjection;
    bool   		NewData;

    Ioniser() :
		Installed( false ),
		Mode( 0 ),
		Status( 0 ),
		Duration( 0 ),
		Current( 0 ),
		CurrentMode( 0 ),
		LastInjection( 0 ),
		NewData( false )
	{
    }
};


struct AutoChlor
{
    bool        Installed;
    int			Mode;
    int			Status;
    int			AcidStatus;
    int         GasDuration;
    int			GasLastInjection;
    bool   		NewData;

    AutoChlor() :
		Installed( false ),
		Mode( 0 ),
		Status( 0 ),
		AcidStatus( 0 ),
		GasDuration( 0 ),
		GasLastInjection( 0 ),
		NewData( false )
	{
    }
};

struct HistoryData
{
	int 	NumBackwashes;
	XmlDate LastBackwashDate;
	XmlTime LastBackwashTime;
	int		NumRefills;
	XmlDate LastRefillDate;
	XmlTime LastRefillTime;
	XmlDate LastPhDate;
	XmlTime LastPhTime;
	bool	NewData;

	HistoryData() :
		NumBackwashes( 0 ),
		LastBackwashDate( "0000-00-00" ),
		LastBackwashTime( "00:00:00" ),
		NumRefills( 0 ),
		LastRefillDate( "0000-00-00" ),
		LastRefillTime( "00:00:00" ),
		LastPhDate( "0000-00-00" ),
		LastPhTime( "00:00:00" ),
		NewData( false )
	{}
};


class PoolCopData
{
    CritSec                 m_critsec;

public:
    ConfigurationData       configData;
    DynamicData             dynamicData;
    PoolSettings            poolSettings;
    FilterSettings          filterSettings;
    PumpSettings			pumpSettings;
    Auxiliaries             auxSettings;
    Inputs					inputs;
    WaterLevelControl       waterLevelControl;
    PhControl               pHControl;
    ORPControl              orpControl;
    Ioniser                 ioniser;
    AutoChlor               autoChlor;
    HistoryData				historyData;

    bool					fRefreshPending;
    bool					fDataRateChanged;
    bool                    fInitialDataRefresh;

    char					zLastRxMsg[128];
    DWORD					dwLastRxTime;

    PoolCopData()
    {
    }

    void Lock()             { m_critsec.Lock(); }
    void Unlock()           { m_critsec.Unlock(); }
};

extern PoolCopData  g_PoolCopData;

#endif
