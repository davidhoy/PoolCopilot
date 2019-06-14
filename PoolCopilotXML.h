/*
 * PoolCopilotXML.h
 *
 *  Created on: Oct 31, 2011
 *      Author: david
 */


#include "Tinyxml/TinyXML.h"
#include "PoolcopDataBindings.h"

#if !defined(DWORD)
#define DWORD	unsigned long
#endif

#define XML_CONFIG_DATA			0x0001
#define XML_DYNAMIC_DATA 		0x0002
#define XML_POOL_SETTINGS 		0x0004
#define XML_FILTER_SETTINGS 	0x0008
#define XML_PUMP_SETTINGS  		0x0010
#define XML_AUXILIARIES 		0x0020
#define XML_WATER_LEVEL_CONTROL 0x0040
#define XML_PH_CONTROL 			0x0080
#define XML_ORP_CONTROL 		0x0100
#define XML_IONISER 			0x0200
#define XML_AUTOCHLOR 			0x0400
#define XML_HISTORY				0x0800
#define XML_INPUTS				0x1000
#define XML_ONLY_NEW_DATA    	0x8000

//#define XML_IMPORTANT_DATA		(XML_DYNAMIC_DATA)
#define XML_ALL_SETTINGS		(XML_POOL_SETTINGS | XML_FILTER_SETTINGS | XML_PUMP_SETTINGS | XML_AUXILIARIES | \
		                         XML_WATER_LEVEL_CONTROL | XML_PH_CONTROL | XML_ORP_CONTROL | XML_IONISER | \
		                         XML_AUTOCHLOR | XML_HISTORY | XML_INPUTS )
#define XML_EVERYTHING			(XML_CONFIG_DATA | XML_DYNAMIC_DATA | XML_ALL_SETTINGS )


TiXmlElement* NetworkConfigurationToXml    ( char *pzName,
		                                     NetworkConfiguration *pNetworkConfig,
		                                     DWORD dwFlags );

TiXmlElement* PoolCopilotConfigurationToXml( char *pzName,
		                                     PoolCopilotConfiguration *pConfig,
		                                     DWORD dwFlags );

TiXmlElement* DynamicDataToXml             ( char *pzName,
		                                     DynamicData *pData,
		                                     DWORD dwFlags );

TiXmlElement* PoolSettingsToXml            ( char *pzName,
										     PoolSettings *pSettings,
										     DWORD dwFlags );

TiXmlElement* PumpSettingsToXml            ( char *pzName,
											 PumpSettings *pSettings,
											 DWORD dwFlags );

TiXmlElement* ConfigurationDataToXml       ( char *pzName,
										     ConfigurationData *pConfig,
										     DWORD dwFlags );

TiXmlElement* TimerCycleToXml              ( char *pzName,
		                                     TimerCycle *pTimer );

TiXmlElement* FilterSettingsToXml          ( char *pzName,
		                                     FilterSettings *pSettings,
		                                     DWORD dwFlags );

TiXmlElement* AuxiliaryToXml               ( char *pzName,
		                                     Auxiliary *pAux,
		                                     DWORD dwFlags );

TiXmlElement* AuxiliariesToXml             ( char *pzName,
		                                     Auxiliaries *pAuxiliaries,
		                                     DWORD dwFlags );

TiXmlElement* InputsToXml                  ( char *pzName,
		                                     Inputs *pInputs,
		                                     DWORD dwFlags );

TiXmlElement* WaterLevelControlToXml       ( char *pzName,
										     WaterLevelControl *pControl,
										     DWORD dwFlags );

TiXmlElement* PhControlToXml               ( char *pzName,
		                                     PhControl *pControl,
		                                     DWORD dwFlags );

TiXmlElement* ORPControlToXml              ( char *pzName,
		                                     ORPControl *pControl,
		                                     DWORD dwFlags );

TiXmlElement* IoniserToXml                 ( char *pzName,
											 Ioniser *pIoniser,
											 DWORD dwFlags );

TiXmlElement* AutoChlorToXml               ( char *pzName,
											 AutoChlor *pAutoChlor,
											 DWORD dwFlags );

TiXmlElement* HistoryToXml                 ( char *pzName,
											 HistoryData *pHistory,
											 DWORD dwFlags );

/*
TiXmlElement* AlarmToXml                   ( char *pzName,
										     Alarm *pAlarm,
										     DWORD dwFlags );

TiXmlElement* AlarmsToXml                  ( char *pzName,
											 Alarms *pAlarms,
											 DWORD dwFlags );
*/

TiXmlElement* PoolCopDataToXml             ( char         *pzName,
		                                     PoolCopData  *pData,
		                                     DWORD        dwFlags,
		                                     TiXmlElement *pExtraXmlNode);

TiXmlDocument* CreatePoolCopilotXml( PoolCopData  *pPoolCopData,
                                     DWORD        dwFlags,
                                     char         *pszMessage,
                                     TiXmlElement *pExtraXmlNode );
