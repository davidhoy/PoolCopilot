/*
 * PoolCopilotXML.cpp
 *
 *  Created on: Oct 31, 2011
 *      Author: david
 */

#include <utils.h>
#include "PoolCopilotXml.h"
#include "Utilities.h"
#include "ConfigData.h"


TiXmlElement* NetworkConfigurationToXml( char *pzName,
		                                 NetworkConfiguration *pConfig,
		                                 DWORD dwFlags )
{
	if ( !(dwFlags & XML_CONFIG_DATA) ||
	     ((dwFlags & XML_ONLY_NEW_DATA) && (pConfig->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
	{
		pElement->SetAttribute( "UseDHCP",        pConfig->UseDHCP );
		pElement->SetAttribute( "IPAddress",      pConfig->IPAddress.c_str() );
        pElement->SetAttribute( "NetMask",        pConfig->NetMask.c_str() );
        pElement->SetAttribute( "DNSAddress",     pConfig->DNSAddress.c_str() );
        pElement->SetAttribute( "GatewayAddress", pConfig->GatewayAddress.c_str() );
        pElement->SetAttribute( "MACAddress",     pConfig->MACAddress.c_str() );
        pConfig->NewData = false;
    }
	return pElement;
}


TiXmlElement* PoolCopilotConfigurationToXml( char *pzName,
		                                     PoolCopilotConfiguration *pConfig,
		                                     DWORD dwFlags )
{
	if ( !(dwFlags & XML_CONFIG_DATA) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pConfig->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
	{
        pElement->SetAttribute( "ServerURL",    g_ConfigData.GetStringValue( "ServerUrl" ) );
        pElement->SetAttribute( "AltServerURL", g_ConfigData.GetStringValue( "AltServerUrl" ) );
        pElement->SetAttribute( "ResetCounter", pConfig->ResetCounter );

        pConfig->NewData = false;
    }
    return pElement;
}



TiXmlElement* DynamicDataToXml( char *pzName,
		                        DynamicData *pData,
                                DWORD dwFlags )
{
	if ( !(dwFlags & XML_DYNAMIC_DATA) ||
	     ((dwFlags & XML_ONLY_NEW_DATA) && (pData->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
    	char szTemp[40];

        pElement->SetAttribute( "Status",          		  pData->PoolCopStatus );
        pElement->SetAttribute( "PumpState",              pData->PumpState );
        pElement->SetAttribute( "ValvePosition",          pData->ValvePosition );
        sprintf( szTemp, "%0.1f", pData->PumpPressure );
        pElement->SetAttribute( "PumpPressure",           szTemp );
        sprintf( szTemp, "%0.1f", pData->pH );
        pElement->SetAttribute( "pH",                     szTemp );
        sprintf( szTemp, "%0.1f", pData->BatteryVoltage );
        pElement->SetAttribute( "BatteryVoltage",         szTemp );
        sprintf( szTemp, "%0.1f", pData->WaterTemperature );
        pElement->SetAttribute( "WaterTemperature",       szTemp );
        pElement->SetAttribute( "ORP",                    pData->Orp );
        pElement->SetAttribute( "Input1State",            pData->Input1State );
        pElement->SetAttribute( "Input1Function",         pData->Input1Function );
        pElement->SetAttribute( "Input2State",            pData->Input2State );
        pElement->SetAttribute( "Input2Function",         pData->Input2Function );
        pElement->SetAttribute( "MainsPowerLost",         pData->MainsPowerLost );
        pElement->SetAttribute( "PumpCurrentSpeed",       pData->PumpCurrentSpeed );
        pElement->SetAttribute( "PumpForcedHrsRemaining", pData->PumpForcedHrsRemaining );
        pElement->SetAttribute( "RunningStatus",          pData->RunningStatus );
        pElement->SetAttribute( "ValveRotationsCounter",  pData->ValveRotationsCounter );
        pElement->SetAttribute( "AirTemperature",         pData->AirTemperature );
        pElement->SetAttribute( "MainsVoltage",           pData->MainsVoltage );

        szTemp[0] = '\0';
        for ( int i = 0; i < pData->NumAlarms; i++ )
        {
        	strcat( szTemp, pData->Alarms[i] ? "1" : "0" );
        } /* for */
        pElement->SetAttribute( "Alarms", szTemp );

        sprintf( szTemp, "%04d/%02d/%02d %02d:%02d:%02d",
                		 pData->Date.Year, pData->Date.Month, pData->Date.Day,
                		 pData->Time.Hour, pData->Time.Minute, pData->Time.Second );
        pElement->SetAttribute( "DateTime",            szTemp );

        pData->NewData = false;
    }
    return pElement;
}


TiXmlElement* PoolSettingsToXml( char *pzName,
		                         PoolSettings *pSettings,
                                 DWORD dwFlags )
{
	if ( !(dwFlags & XML_POOL_SETTINGS) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pSettings->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        pElement->SetAttribute( "PoolVolume",        pSettings->PoolVolume );
        pElement->SetAttribute( "FlowRate",          pSettings->FlowRate );
        pElement->SetAttribute( "TurnoverPerDay",    pSettings->TurnoversPerDay );
        pElement->SetAttribute( "FreezeProtection",  pSettings->FreezeProtection );
        pElement->SetAttribute( "CoverReductionPct", pSettings->CoverReductionPct );
        pElement->SetAttribute( "PoolType",          pSettings->PoolType );
		pElement->SetAttribute( "ServiceMode",       pSettings->ServiceMode );
		pElement->SetAttribute( "CoverSpeed",        pSettings->CoverSpeed );

        pSettings->NewData = false;
    }
    return pElement;
}


TiXmlElement* PumpSettingsToXml( char *pzName,
		                         PumpSettings *pSettings,
                                 DWORD dwFlags )
{
	if ( !(dwFlags & XML_PUMP_SETTINGS) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pSettings->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement )
    {
        pElement->SetAttribute( "PressureLow",    pSettings->PressureLow );
        pElement->SetAttribute( "AlarmPressure",  pSettings->AlarmPressure );
        pElement->SetAttribute( "PumpProtect",    pSettings->PumpProtect );
        pElement->SetAttribute( "PumpType",       pSettings->PumpType );
        pElement->SetAttribute( "SpeedCycle1",    pSettings->SpeedCycle1 );
        pElement->SetAttribute( "SpeedCycle2",    pSettings->SpeedCycle2 );
        pElement->SetAttribute( "SpeedBackwash",  pSettings->SpeedBackwash );
        pElement->SetAttribute( "NbSpeeds",       pSettings->NumSpeeds );
        pElement->SetAttribute( "ForcedPumpMode", pSettings->ForcedPumpMode );

        pSettings->NewData = false;
    }
    return pElement;
}


TiXmlElement* ConfigurationDataToXml( char *pzName,
		                              ConfigurationData *pConfig,
	                                  DWORD dwFlags )
{
	if ( !(dwFlags & XML_CONFIG_DATA) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pConfig->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        TiXmlElement *pSubElement = NetworkConfigurationToXml( "NetworkConfiguration", &pConfig->networkConfig, dwFlags );
     	if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = PoolCopilotConfigurationToXml( "PoolCopilotConfiguration", &pConfig->poolcopilotConfig, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pElement->SetAttribute( "PoolCopVersion",     pConfig->PoolCopVersion.c_str() );
        pElement->SetAttribute( "PoolCopilotVersion", pConfig->PoolCopilotVersion.c_str() );
		pElement->SetAttribute( "HardwareType",       pConfig->HardwareType.c_str() );

        pConfig->NewData = false;
    }
    return pElement;
}


TiXmlElement* TimerCycleToXml( char *pzName,
							   TimerCycle *pTimer )
{
    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
    	char	szTemp[40];

        pElement->SetAttribute( "Enabled", pTimer->Enabled );

        sprintf( szTemp, "%02d:%02d:%02d",
             	 pTimer->TimeOn.Hour, pTimer->TimeOn.Minute, pTimer->TimeOn.Second );
        pElement->SetAttribute( "TimeOn",  szTemp );

        sprintf( szTemp, "%02d:%02d:%02d",
                 pTimer->TimeOff.Hour, pTimer->TimeOff.Minute, pTimer->TimeOff.Second );
        pElement->SetAttribute( "TimeOff", szTemp );
    }
    return pElement;
}


TiXmlElement* FilterSettingsToXml( char *pzName,
		                           FilterSettings *pSettings,
                                   DWORD dwFlags )
{
	if ( !(dwFlags & XML_FILTER_SETTINGS) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pSettings->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        pElement->SetAttribute( "Pressure",               pSettings->Pressure );
        pElement->SetAttribute( "BackwashDuration",       pSettings->BackwashDuration );
        pElement->SetAttribute( "RinseDuration",          pSettings->RinseDuration );
        pElement->SetAttribute( "AutoBackwash",           pSettings->AutoBackwash );
        pElement->SetAttribute( "MaxDaysBetweenBackwash", pSettings->MaxDaysBetweenBackwash );
        pElement->SetAttribute( "WasteLineValve",         pSettings->WasteLineValve );
        pElement->SetAttribute( "TimerMode",              pSettings->TimerMode );

        TiXmlElement *pTimer = TimerCycleToXml( "TimerCycle1", &pSettings->TimerCycle1 );
        if ( pTimer )
        {
        	pElement->LinkEndChild( pTimer );
        }
        pTimer = TimerCycleToXml( "TimerCycle2", &pSettings->TimerCycle2 );
        if ( pTimer )
        {
            pElement->LinkEndChild( pTimer );
        }

        char	szTemp[20];
        sprintf( szTemp, "%0.1f", pSettings->MaxTemp );
        pElement->SetAttribute( "PoolMaxTemp", szTemp );
        sprintf( szTemp, "%0.1f", pSettings->MinTemp );
        pElement->SetAttribute( "PoolMinTemp", szTemp );

        pSettings->NewData = false;
    }
    return pElement;
}


TiXmlElement* AuxiliaryToXml( char *pzName,
		                      Auxiliary *pAux,
                              DWORD dwFlags )
{
	if ( !(dwFlags & XML_AUXILIARIES) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pAux->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
    	char szTemp[40];

    	pElement->SetAttribute( "Label",         pAux->Label );
        pElement->SetAttribute( "Slave",         pAux->Slave );
        pElement->SetAttribute( "Status",        pAux->Status );
        pElement->SetAttribute( "TimerStatus",   pAux->TimerStatus );

        sprintf( szTemp, "%02d:%02d:%02d",
                 pAux->TimeOn.Hour, pAux->TimeOn.Minute, pAux->TimeOn.Second );
        pElement->SetAttribute( "TimeOn",        szTemp );

        sprintf( szTemp, "%02d:%02d:%02d",
                 pAux->TimeOff.Hour, pAux->TimeOff.Minute, pAux->TimeOff.Second );
        pElement->SetAttribute( "TimeOff",       szTemp );

        pElement->SetAttribute( "DaysOfWeek",    pAux->DaysOfWeek.c_str() );
        pAux->NewData = false;
    }
    return pElement;
}


TiXmlElement* AuxiliariesToXml( char *pzName,
		                        Auxiliaries *pAuxiliaries,
                                DWORD dwFlags )
{
	if ( !(dwFlags & XML_AUXILIARIES) ||
	     ((dwFlags & XML_ONLY_NEW_DATA) && (pAuxiliaries->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        pElement->SetAttribute( "AuxCount", AUX_COUNT );
    	for ( int i = 0; i < AUX_COUNT; i++ )
    	{
    		char szName[10];
    		sprintf( szName, "Aux%d", i+1 );
    		TiXmlElement *pAux = AuxiliaryToXml( szName, &pAuxiliaries->Aux[i], dwFlags );
    		if ( pAux != NULL )
    		{
    			pElement->LinkEndChild( pAux );
    		}
    		pAuxiliaries->NewData = false;
    	} /* for */
    }
    return pElement;
}


TiXmlElement* InputsToXml( char *pzName,
		                   Inputs *pInputs,
                           DWORD dwFlags )
{
	if ( !(dwFlags & XML_INPUTS) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pInputs->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
       	pElement->SetAttribute( "Input1Dir",  pInputs->Input1Dir  );
    	pElement->SetAttribute( "Input1Func", pInputs->Input1Func );
    	pElement->SetAttribute( "Input2Dir",  pInputs->Input2Dir  );
    	pElement->SetAttribute( "Input2Func", pInputs->Input2Func );
        pInputs->NewData = false;
    }
    return pElement;
}


TiXmlElement* WaterLevelControlToXml( char *pzName,
		                              WaterLevelControl *pControl,
	                                  DWORD dwFlags  )
{
	if ( !(dwFlags & XML_WATER_LEVEL_CONTROL) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pControl->NewData == false)) )
	{
		return NULL;
	}

    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        pElement->SetAttribute( "Installed",       pControl->Installed );
        pElement->SetAttribute( "WaterValve",      pControl->WaterValve );
        pElement->SetAttribute( "WaterLevel",      pControl->WaterLevel );
        pElement->SetAttribute( "AutoWaterAdd",    pControl->AutoWaterAdd );
        pElement->SetAttribute( "CableStatus",     pControl->CableStatus );
        pElement->SetAttribute( "ContinuousFill",  pControl->ContinuousFill );
        pElement->SetAttribute( "MaxFillDuration", pControl->MaxFillDuration );
        pElement->SetAttribute( "AutoWaterReduce", pControl->AutoWaterReduce );
        pElement->SetAttribute( "DrainingDuration",pControl->DrainingDuration );

        pControl->NewData = false;
    }
    return pElement;
}


TiXmlElement* PhControlToXml( char *pzName,
		                      PhControl *pControl,
                              DWORD dwFlags )
{
	if ( !(dwFlags & XML_PH_CONTROL) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pControl->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
	{
		char 	szTemp[40];

        pElement->SetAttribute( "Installed",     pControl->Installed );
        pElement->SetAttribute( "Status",        pControl->Status );
        sprintf( szTemp, "%0.1f", pControl->SetPoint );
        pElement->SetAttribute( "SetPoint",      szTemp );
        pElement->SetAttribute( "pHType",        pControl->pHType );
        pElement->SetAttribute( "DosingTime",    pControl->DosingTime );
        pElement->SetAttribute( "LastInjection", pControl->LastInjection );

        pControl->NewData = false;
    }
    return pElement;
}


TiXmlElement* ORPControlToXml( char *pzName,
		                       ORPControl *pControl,
                               DWORD dwFlags )
{
	if ( !(dwFlags & XML_ORP_CONTROL) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pControl->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
    {
        pElement->SetAttribute( "Installed",               pControl->Installed );
        pElement->SetAttribute( "SetPoint",                pControl->SetPoint );
        pElement->SetAttribute( "DisinfectantType",        pControl->DisinfectantType );
        pElement->SetAttribute( "OrpControl",              pControl->OrpControl );
        pElement->SetAttribute( "LastInjection",           pControl->LastInjection );
        pElement->SetAttribute( "HyperchlorationSetPoint", pControl->HyperchlorationSetPoint );
        pElement->SetAttribute( "HyperchlorationWeekday",  pControl->HyperchlorationWeekday );
        pElement->SetAttribute( "LowShutdownTemp",         pControl->LowShutdownTemp );


        pControl->NewData = false;
    }
    return pElement;
}


TiXmlElement* IoniserToXml( char *pzName,
		                    Ioniser *pIoniser,
                            DWORD dwFlags )
{
	if ( !(dwFlags & XML_IONISER) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pIoniser->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
        pElement->SetAttribute( "Installed",     pIoniser->Installed );
        pElement->SetAttribute( "Mode",          pIoniser->Mode );
        pElement->SetAttribute( "Status",        pIoniser->Status );
        pElement->SetAttribute( "Duration",      pIoniser->Duration );
        pElement->SetAttribute( "Current",       pIoniser->Current );
        pElement->SetAttribute( "CurrentMode",   pIoniser->CurrentMode );
        pElement->SetAttribute( "LastInjection", pIoniser->LastInjection );

        pIoniser->NewData = false;
    }
    return pElement;
}


TiXmlElement* AutoChlorToXml( char *pzName,
		                      AutoChlor *pAutoChlor,
                              DWORD dwFlags )
{
	if ( !(dwFlags & XML_AUTOCHLOR) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pAutoChlor->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
    {
        pElement->SetAttribute( "Installed",        pAutoChlor->Installed );
        pElement->SetAttribute( "Mode",             pAutoChlor->Mode );
        pElement->SetAttribute( "Status",           pAutoChlor->Status );
        pElement->SetAttribute( "AcidStatus",       pAutoChlor->AcidStatus );
        pElement->SetAttribute( "GasDuration",      pAutoChlor->GasDuration );
        pElement->SetAttribute( "GasLastInjection", pAutoChlor->GasLastInjection );

        pAutoChlor->NewData = false;
    }
    return pElement;
}


TiXmlElement* HistoryToXml( char *pzName,
		                    HistoryData *pHistory,
                            DWORD dwFlags )
{
	if ( !(dwFlags & XML_HISTORY) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pHistory->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
    {
		char	szDate[40];

		sprintf( szDate, "%04d-%02d-%02d %02d:%02d:%02d",
				 pHistory->LastBackwashDate.Year,
				 pHistory->LastBackwashDate.Month,
				 pHistory->LastBackwashDate.Day,
				 pHistory->LastBackwashTime.Hour,
				 pHistory->LastBackwashTime.Minute,
				 pHistory->LastBackwashTime.Second );
        pElement->SetAttribute( "NumBackwashes",    pHistory->NumBackwashes );
        pElement->SetAttribute( "LastBackwashDate", szDate );

        sprintf( szDate, "%04d-%02d-%02d %02d:%02d:%02d",
        		 pHistory->LastRefillDate.Year,
        		 pHistory->LastRefillDate.Month,
        		 pHistory->LastRefillDate.Day,
        		 pHistory->LastRefillTime.Hour,
        		 pHistory->LastRefillTime.Minute,
        		 pHistory->LastRefillTime.Second );
        pElement->SetAttribute( "NumRefills",       pHistory->NumRefills );
		pElement->SetAttribute( "LastRefillDate",   szDate );

	    sprintf( szDate, "%04d-%02d-%02d %02d:%02d:%02d",
	        	 pHistory->LastPhDate.Year,
	        	 pHistory->LastPhDate.Month,
	        	 pHistory->LastPhDate.Day,
	        	 pHistory->LastPhTime.Hour,
	        	 pHistory->LastPhTime.Minute,
	        	 pHistory->LastPhTime.Second );
		pElement->SetAttribute( "LastPhDate", szDate );

        pHistory->NewData = false;
    }
    return pElement;
}

#if 0
TiXmlElement* AlarmToXml( char *pzName,
		                  Alarm *pAlarm,
                          DWORD dwFlags )
{
	if ( !(dwFlags & XML_ALARMS) ||
		 ((dwFlags & XML_ONLY_NEW_DATA) && (pAlarm->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
    {
        char szTemp[40];

        pElement->SetAttribute( "Name",  pAlarm->Name );

        sprintf( szTemp, "%04d-%02d-%02d",
                 pAlarm->Date.Year, pAlarm->Date.Month, pAlarm->Date.Day );
        pElement->SetAttribute( "Date", szTemp );

        sprintf( szTemp, "%02d:%02d:%02d",
                 pAlarm->Time.Hour, pAlarm->Time.Minute, pAlarm->Time.Second );
        pElement->SetAttribute( "Time", szTemp );

        pElement->SetAttribute( "Status", pAlarm->Status );

        pAlarm->NewData = false;
    }
    return pElement;
}


TiXmlElement* AlarmsToXml( char *pzName,
		                   Alarms *pAlarms,
                           DWORD dwFlags )
{
	if ( !(dwFlags & XML_ALARMS) ||
	     ((dwFlags & XML_ONLY_NEW_DATA) && (pAlarms->NewData == false)) )
	{
		return NULL;
	}

	TiXmlElement *pElement = new TiXmlElement( pzName );
	if ( pElement != NULL )
    {
		for ( int i = 0; i < ALARM_COUNT; i++ )
		{
			char szName[20];
			sprintf( szName, "Alarm%d", i+1 );
			TiXmlElement *pAlarm = AlarmToXml( szName,  &pAlarms->alarm[i], dwFlags );
			if ( pAlarm != NULL )
			{
				pElement->LinkEndChild( pAlarm );
			}

			pAlarms->NewData = false;
		} /* for */
    }
    return pElement;
}
#endif

TiXmlElement* PoolCopDataToXml( char         *pzName,
		                        PoolCopData  *pData,
		                        DWORD        dwFlags,
		                        TiXmlElement *pExtraXmlNode )
{
    TiXmlElement *pElement = new TiXmlElement( pzName );
    if ( pElement != NULL )
    {
    	TiXmlElement *pSubElement;

        pElement->SetAttribute( "MACAddress", pData->configData.networkConfig.MACAddress.c_str() );

    	pSubElement = ConfigurationDataToXml( "ConfigurationData", &pData->configData, dwFlags );
    	if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = DynamicDataToXml( "DynamicData", &pData->dynamicData, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = PoolSettingsToXml( "PoolSettings", &pData->poolSettings, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = FilterSettingsToXml( "FilterSettings", &pData->filterSettings, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = PumpSettingsToXml( "PumpSettings", &pData->pumpSettings, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = AuxiliariesToXml( "Auxiliaries", &pData->auxSettings, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = InputsToXml( "Inputs", &pData->inputs, dwFlags );
		if ( pSubElement )
		{
			pElement->LinkEndChild( pSubElement );
		}
        pSubElement = WaterLevelControlToXml( "WaterLevelControl", &pData->waterLevelControl, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = PhControlToXml( "pHControl", &pData->pHControl, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = ORPControlToXml( "ORPControl", &pData->orpControl, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = IoniserToXml( "Ioniser", &pData->ioniser, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = AutoChlorToXml( "AutoChlor", &pData->autoChlor, dwFlags );
        if ( pSubElement )
        {
            pElement->LinkEndChild( pSubElement );
        }
        pSubElement = HistoryToXml( "History", &pData->historyData, dwFlags );
		if ( pSubElement )
		{
			pElement->LinkEndChild( pSubElement );
		}
        //pSubElement = AlarmsToXml( "Alarms", &pData->alarms, dwFlags );
        //if ( pSubElement )
        //{
        //	pElement->LinkEndChild( pSubElement );
        //}
		if ( pExtraXmlNode )
		{
			pElement->LinkEndChild( pExtraXmlNode );
		}
    }
    return pElement;
}


TiXmlDocument* CreatePoolCopilotXml( PoolCopData  *pData,
		                             DWORD        dwFlags,
		                             char         *pszMessage,
		                             TiXmlElement *pExtraXmlNode )
{
	TiXmlDocument *pDoc = new TiXmlDocument();
	if ( pDoc == NULL )
    {
        return NULL;
    }
	TiXmlDeclaration *pDecl = new TiXmlDeclaration( "1.0", "UTF-8", "yes" );
	if ( pDecl )
    {
        pDoc->LinkEndChild( pDecl );
    }
	TiXmlElement *pElement = PoolCopDataToXml( "PoolCopilot",
			                                   pData,
			                                   dwFlags,
			                                   pExtraXmlNode );
	if ( pElement )
    {
		pElement->SetAttribute( "Refresh",        pData->fInitialDataRefresh );
        pElement->SetAttribute( "RefreshPending", pData->fRefreshPending );
		if ( pszMessage )
		{
			pElement->SetAttribute( "Message", pszMessage );
		}
		pElement->SetAttribute( "LastMessageReceived",  pData->zLastRxMsg );
		pElement->SetAttribute( "LastMessageTimestamp", Secs - pData->dwLastRxTime );
		pDoc->LinkEndChild( pElement );
    }

	return pDoc;
}
