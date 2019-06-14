/*
 * PoolCopilot.cpp
 *
 *  Created on: Jul 2, 2011
 *      Author: david
 */

#include <stdarg.h>
#include "PoolCopilot.h"
#include "TaskPriorities.h"
#include "SHA1.h"
#include "PoolCopDataBindings.h"
#include "PoolCopilotXml.h"
#include "PoolCopInterface.h"
#include "ConfigData.h"
#include "Utilities.h"
#include "Console.h"
#include "FirmwareUpdate.h"
#include "Version.h"


static DWORD g_PoolCopilotTaskStack[USER_TASK_STK_SIZE];
extern bool g_fUsingDhcp;

#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_POOLCOPILOT, level, fmt, ##__VA_ARGS__ )

typedef int (*PFN_PARSERFUNC)( TiXmlElement *pCommand );
typedef struct
{
	const char 			   *pszCommand;
	const PFN_PARSERFUNC	pfnParser;
} PARSE_TBL_ENTRY, *PPARSE_TBL_ENTRY;

static const PARSE_TBL_ENTRY ParserTbl[] =
{
	{ "SetTime",     PoolCopilot::ParseSetTime },
	{ "ControlPump", PoolCopilot::ParseControlPump },
    { NULL, NULL }
};


/********************************************************************
* PoolCopilot::PoolCopilot()
*
* Default constructor for PoolCopilot class
*
* Parameters:   N/A
*
* Return:       N/A
*********************************************************************/
PoolCopilot::PoolCopilot()
{
	m_iHttpKeepAlive      = 15;
	m_fThreadRunning      = false;
	m_fShutdownThread     = false;
	m_fUseAltServer       = false;
	m_pResultsXml         = NULL;
	m_pServer             = NULL;
	m_iServerBackoff      = 4;

} /* PoolCopilot::PoolCopilot() */


/********************************************************************
* PoolCopilot::~PoolCopilot()
*
* Default destructor for PoolCopilot class
*
* Parameters:   N/A
*
* Return:       N/A
*********************************************************************/
PoolCopilot::~PoolCopilot()
{
	// Signal thread to shutdown and wait for it to exit.
	//
	m_fShutdownThread = true;
	while ( m_fThreadRunning )
	{
		OSTimeDly( TICKS_PER_SECOND );
	} /* while */

	if ( m_pServer )
		delete m_pServer;

} /* PoolCopilot::~PoolCopilot() */


/********************************************************************
* PoolCopilot::Startup()
*
* This method is called to initialize the object and start the thread
* that connects to the server, sends XML over HTTP, receives the
* response and parses it.
*
* Parameters:   void
*
* Return:       int
*********************************************************************/
int PoolCopilot::Startup( void )
{
	int 	iRet;

	g_PoolCopData.configData.networkConfig.UseDHCP        = g_fUsingDhcp;
	g_PoolCopData.configData.networkConfig.IPAddress      = IPToStr( EthernetIP );
	g_PoolCopData.configData.networkConfig.NetMask        = IPToStr( EthernetIpMask );
	g_PoolCopData.configData.networkConfig.DNSAddress     = IPToStr( EthernetDNS );
	g_PoolCopData.configData.networkConfig.GatewayAddress = IPToStr( EthernetIpGate );
	g_PoolCopData.configData.networkConfig.MACAddress     = MacToStr( (unsigned char*)gConfigRec.mac_address );
    g_PoolCopData.configData.networkConfig.NewData        = true;

	g_PoolCopData.configData.poolcopilotConfig.ServerURL          = g_ConfigData.GetStringValue( "ServerUrl" );
	g_PoolCopData.configData.poolcopilotConfig.AltServerURL       = g_ConfigData.GetStringValue( "AltServerUrl" );
	g_PoolCopData.configData.poolcopilotConfig.ConnectionInterval = g_ConfigData.GetIntValue( "ConnectionInterval" );
    g_PoolCopData.configData.poolcopilotConfig.ResetCounter       = g_ConfigData.GetIntValue( "ResetCounter" );
    g_PoolCopData.configData.poolcopilotConfig.NewData            = true;

	g_PoolCopData.configData.PoolCopilotVersion = VERSION_STRING;
    g_PoolCopData.configData.NewData            = true;

    m_pServer = new ServerInterface();

	DebugTrace( 16, "%s: enter\n", __FUNCTION__ );
	iRet = OSTaskCreate( TaskThreadWrapper,
						 (void*)this,
						 (void*)&g_PoolCopilotTaskStack[USER_TASK_STK_SIZE],
						 (void*)g_PoolCopilotTaskStack,
						 POOLCOPILOT_PRIO );
	if ( iRet != OS_NO_ERR )
	{
		DebugTrace( 1, "%s: Error starting thread, err=%d\n", __FUNCTION__, iRet );
	}

	return iRet;

} /* PoolCopilot::Startup() */


/********************************************************************
* PoolCopilot::Shutdown()
*
* This method does any termination logic and cleanup
*
* Parameters:   void
*
* Return:       int
*********************************************************************/
int	PoolCopilot::Shutdown( void )
{

	if ( m_pServer )
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	return 0;

} /* PoolCopilot::Shutdown() */


/********************************************************************
* PoolCopilot::TaskThreadWrapper()
*
* Static class method passed as an argument to OSTaskCreate().  It
* simply extracts the "this" pointer from the single argument, and
* then calls the regular class method to actually run the thread.
*
* Parameters:   pArgs - 'this' pointer
*
* Return:       void
*********************************************************************/
void PoolCopilot::TaskThreadWrapper( void *pArgs )
{
	// This is a static function, so we get the 'this' pointer from
    // the args and call the member method.
	//
    PoolCopilot* pThis = (PoolCopilot*)pArgs;
	pThis->TaskThread();

} /* PoolCopilot::TaskThreadWrapper() */


/********************************************************************
* PoolCopilot::TaskThread()
*
* This method is the main task thread that connects to the server,
* sends XML over HTTP, receives the response and parses it.
*
* Parameters:   void
*
* Return:       void
*********************************************************************/
void PoolCopilot::TaskThread( void )
{
	m_fThreadRunning = true;


	// Initial startup delay of 5 sec
	//
	OSTimeDly( TICKS_PER_SECOND * 5 );


	// Main thread loop, do handshaking, receive messages, transmit messages, etc.
	//
	DWORD dwFlags = XML_EVERYTHING;
	while ( !m_fShutdownThread )
	{
        // Connect to the server.
        //
		if ( !m_pServer->IsConnected() )
		{
			ConnectToServer();
		} /* if */

        // Once we're connect, send the XML message to the server.
        //
		if ( m_pServer->IsConnected() )
		{
			int iRet = SendDataToServer( dwFlags );
			if ( iRet > 0 )
			{
			//	dwFlags |= XML_ONLY_NEW_DATA;
			} /* if */
			else
			{
				m_pServer->Close();
			//	dwFlags &= ~XML_ONLY_NEW_DATA;
			}
		} /* if */

		DebugTrace(6, "\r\nWaiting %d seconds before reconnecting to server\r\n", m_iServerBackoff);
		OSTimeDly( m_iServerBackoff * TICKS_PER_SECOND );
	} /* while */

	m_fThreadRunning = false;

} /* PoolCopInterface::TaskThread() */


/********************************************************************
* PoolCopilot::ConnectToServer()
*
* This method connects to the server, using either the primary or
* alternate URLs.
*
* Parameters:   void
*
* Return:       int - 0 for success, else error code.
*********************************************************************/
int PoolCopilot::ConnectToServer( void )
{
	int iRet = -1;
    const char* pszUrl;


    // Check for dynamic changes to server URLs
    //
    if ( g_ConfigData.HaveUrlsChanged() )
    {
        m_fUseAltServer = false;
        g_ConfigData.UrlsHaveChanged( false );
    } /* if */


	if ( m_fUseAltServer == false )
	{
        pszUrl = g_ConfigData.GetStringValue( "ServerUrl" );
		DebugTrace( 1, "Connecting to primary server %s...", pszUrl );
		iRet = m_pServer->Connect( pszUrl );
		if ( iRet != 0 )
		{
			DebugTrace( 1, "failed\r\n" );
#ifdef USE_ALT_SERVER
			m_fUseAltServer = true;
#endif
		} /* if */
	} /* if */

#ifdef USE_ALT_SERVER
	if ( m_fUseAltServer == true )
	{
        pszUrl = g_ConfigData.GetStringValue( "AltServerUrl" );
		DebugTrace( 1, "Connecting to alternate server %s...", pszUrl );
		iRet = m_pServer->Connect( pszUrl );
		if ( iRet != 0 )
		{
			DebugTrace( 1, "FAILED\r\n" );
			m_fUseAltServer = false;
		} /* if */
	} /* if */
#endif

	if ( iRet == 0 )
	{
		DebugTrace( 1, "connected(%d)!\r\n", m_pServer->Socket() );
	} /* if */
	return iRet;

} /* PoolCopilot::ConnectToServer() */


/********************************************************************
* PoolCopilot::AsyncDataRcvd()
*
* This method sends an abort to the server (i.e. closes the open
* socket) so that we can immediately send a new request.
*
* Parameters:   void
*
* Return:       void
*********************************************************************/
void PoolCopilot::AsyncDataRcvd( void )
{
	if ( m_pServer && m_pServer->IsConnected() )
	{
		m_pServer->Abort();
	} /* if */

} /* PoolCopilot::AsyncDataRcvd() */


/********************************************************************
* PoolCopilot::SendDataToServer()
*
* This method generates an XML document with all the appropriate
* pool data, and sends it to the connected web server.  Once it
* receives the HTTP response, hands that off to be parsed.
*
* Parameters:   dwFlags
*
* Return:       int
*********************************************************************/
int PoolCopilot::SendDataToServer( DWORD dwFlags )
{
	int 	iRet = 0;


	// Serialize the structure to an XML document
	//
	g_PoolCopData.Lock();
	char *pszMessage = NULL;
	if ( !g_PoolCopData.fInitialDataRefresh )
	{
		pszMessage = "No data from PoolCop yet";
		//dwFlags |= XML_ONLY_NEW_DATA;
	} /* if */
	TiXmlDocument *pDoc = CreatePoolCopilotXml( &g_PoolCopData,
												dwFlags,
												pszMessage,
												m_pResultsXml );
	if ( m_pResultsXml )
	{
		m_pResultsXml = NULL;
	} /* if */
	g_PoolCopData.Unlock();

	// Extract the XML string
	//
	TiXmlPrinter printer;
	printer.SetIndent( "  " );
	printer.SetLineBreak( "\r\n" );
	pDoc->Accept( &printer );
	const char *pszXmlStr = printer.CStr();

	// Calculate the SHA1 hash on the XML
	//
	CSHA1   sha1;
	sha1.Update( (unsigned char *)pszXmlStr, strlen(pszXmlStr));
	sha1.Final();
	TCHAR   szSha1[60];
	sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );
	std::string	strExtraHeader = "X-PoolCopilot-Sha1: ";
	strExtraHeader += szSha1;
	strExtraHeader += "\r\n";

	// Post the XML over HTTP and get the response
	//
	int   cbBuffer    = 8192*2;
	char *pszResponse = new char[cbBuffer];
	iRet = m_pServer->HttpPost( strExtraHeader.c_str(),
	 		                    m_iHttpKeepAlive,
					            pszXmlStr,
					            strlen(pszXmlStr),
					            pszResponse,
					            &cbBuffer,
					            60 );
	if ( iRet == 0 )
	{
		DebugTrace( 1, "Got %d byte response from server\r\n", cbBuffer );

		// Parse the response packet
		//
		int iHttpStatus = 0;
		char szHttpResponse[80] = { 0 };
		char *pTmp = strchr( pszResponse, ' ' );
		if ( pTmp )
		{
			sscanf( pTmp, "%d %s", &iHttpStatus, szHttpResponse );
			DebugTrace( 1, "HTTP status = %d, response = \"%s\"\r\n", iHttpStatus, szHttpResponse );
		} /* if */

		// Parse a successful HTTP response
		//

		if ( iHttpStatus == 200 )
		{
			char * pszRespXml = strstr( pszResponse, "<?xml" );
			if ( pszRespXml )
			{
				DebugTrace( 1, "Found XML in server response, processing it...\r\n" );
				DebugTrace( 5, "%s", pszRespXml );
				iRet = ProcessXmlResponse( pszRespXml );
			} /* if */
			m_iServerBackoff = 4;
		} /* if */
		else if (iHttpStatus >= 500 && iHttpStatus <= 599)
		{
			// Server error, back off for 10 seconds
			DebugTrace(1, "Server error %d, backing off for 10 seconds\r\n", iHttpStatus);
			m_iServerBackoff = 10;
		}
	} /* if */
	else
	{
		DebugTrace( 1, "HttpPost returned %d\r\n", iRet );
	} /* else */

	// Clean up allocated resources
	//
	if (pszResponse)
	{
		delete[] pszResponse;
	} /* if */
	if (pDoc)
	{
		delete pDoc;
	} /* if */

	return iRet;

} /* PoolCopilot::SendDataToServer() */


/********************************************************************
* PoolCopilot::ParseXmlResponse()
*
* This method takes the received XML response and parses it, handling
* each of the commands and sends then on to the PoolCop.
*
* Parameters:   pszXmlStr
*
* Return:       int - number of commands processed from the XML
*********************************************************************/
int PoolCopilot::ProcessXmlResponse( const char * pszXmlStr )
{
	int			iCommandCnt = 0;
	const char 	*tmp;


	// Parse the XML document
	//
    TiXmlDocument newdoc;
    newdoc.Parse( pszXmlStr, 0, TIXML_DEFAULT_ENCODING );

    // Get the "Commands" node from the XML document
    //
    TiXmlHandle docHandle( &newdoc );
    TiXmlElement* pCommands = docHandle.FirstChild( "PoolCopilotResponse" ).FirstChild( "Commands" ).ToElement();
    if ( pCommands )
    {
    	// If the results XML fragment already exists, delete it, and
    	// create a new one.
    	//
		if ( m_pResultsXml )
		{
			delete m_pResultsXml;
			m_pResultsXml = NULL;
		} /* if */
    	m_pResultsXml = new TiXmlElement( "CommandResults" );

    	// Iterate through the "Commands" node of the XML document
    	//
        TiXmlElement* pCommand = pCommands->FirstChildElement();
        while ( pCommand )
        {
        	const char* pzCmdStr = pCommand->Value();
        	int         iCmdRet  = 0;

#if 0
        	for ( int i = 0; ParserTbl[i].pszCommand != NULL; i++ )
        	{
        		if ( (stricmp( ParserTbl[i].pszCommand, pzCmdStr ) == 0) &&
        		     (ParserTbl[i].pfnParser != NULL) )
        		{
        			iCmdRet = ParserTbl[i].pfnParser( pCommand );
        			break;
        		} /* if */
        	} /* for */
#else
 			if ( stricmp( pzCmdStr, "ControlPump" ) == 0 )
			{
 				int iState = 0;
				tmp = pCommand->Attribute( "State" );
				if ( tmp ) iState = atoi( tmp );

				iCmdRet = g_PoolCop.ControlPump( iState );
			}

			else if ( stricmp( pzCmdStr, "SetTime" ) == 0 )
			{
				tmp = pCommand->Attribute( "Date" );
				iCmdRet = g_PoolCop.SetDate( tmp );

				tmp = pCommand->Attribute( "Time" );
				iCmdRet = g_PoolCop.SetTime( tmp );
			}

			else if ( stricmp( pzCmdStr, "ControlAux" ) == 0 )
 			{
				int iAux = 0,
				    iState = 0;

				tmp = pCommand->Attribute( "Aux" );
				if ( tmp ) iAux = atoi( tmp );

 				tmp = pCommand->Attribute( "State" );
 				if ( tmp ) iState = atoi( tmp );

 				iCmdRet = g_PoolCop.ControlAux( iAux, iState );
 			}

			else if ( stricmp( pzCmdStr, "WaterLevelAdd" ) == 0 )
			{
				int iState = 0;

				tmp = pCommand->Attribute( "State" );
				if ( tmp ) iState = atoi( tmp );

				iCmdRet = g_PoolCop.WaterLevelAdd( iState );
			}

			else if ( stricmp( pzCmdStr, "BackwashFilter" ) == 0 )
			{
				iCmdRet = g_PoolCop.BackwashFilter();
			}

			else if ( stricmp( pzCmdStr, "ValveRotate" ) == 0 )
			{
				int iValvePos = 0;

				tmp = pCommand->Attribute( "Position" );
				if ( tmp ) iValvePos = atoi( tmp );

				iCmdRet = g_PoolCop.RotateValve( (VALVEPOS)iValvePos );
			}

			else if ( stricmp( pzCmdStr, "MeasurePh" ) == 0 )
			{
				iCmdRet = g_PoolCop.MeasurePh();
			}

			else if ( stricmp( pzCmdStr, "CalibratePh" ) == 0 )
			{
				int iCalibrationValue = 0;

				tmp = pCommand->Attribute( "CalibrationValue" );
				if ( tmp ) iCalibrationValue = atoi( tmp );

				iCmdRet = g_PoolCop.CalibratePh( iCalibrationValue );
			}

			else if ( stricmp( pzCmdStr, "SelectLanguage" ) == 0 )
			{
				char szLanguage[3] = "";

				tmp = pCommand->Attribute( "Language" );
				if ( tmp ) strncpy( szLanguage, tmp, 2 );

				iCmdRet = g_PoolCop.SelectLanguage( szLanguage );
			}

			else if ( stricmp( pzCmdStr, "SetOrpConfig" ) == 0 )
			{
				int iInstalled = 0,
				    iSetPoint = 0,
				    iType = 0,
				    iHyperchlorationSetPoint = 0,
				    iHyperchlorationWeekday = 0,
				    iLowShutdownTemp = 0;

				tmp = pCommand->Attribute( "Installed" );
				if ( tmp ) iInstalled = atoi( tmp );

				tmp = pCommand->Attribute( "SetPoint" );
				if ( tmp ) iSetPoint = atoi( tmp );

				tmp = pCommand->Attribute( "Type" );
				if ( tmp ) iType = atoi( tmp );

				tmp = pCommand->Attribute( "HyperchlorationSetPoint" );
				if ( tmp ) iHyperchlorationSetPoint = atoi( tmp );

				tmp = pCommand->Attribute( "HyperchlorationWeekday" );
				if ( tmp ) iHyperchlorationWeekday = atoi( tmp );

				tmp = pCommand->Attribute( "LowShutdownTemp" );
				if ( tmp ) iLowShutdownTemp = atoi( tmp );

				iCmdRet = g_PoolCop.SetOrpConfig( iInstalled,
												  iSetPoint,
												  iType,
												  iHyperchlorationSetPoint,
												  iHyperchlorationWeekday,
												  iLowShutdownTemp );
			}

			else if ( stricmp( pzCmdStr, "SetIonizerConfig" ) == 0 )
			{
				int iInstalled = 0,
				    iMode = 0,
				    iMaxPct = 0,
				    iCurrentInPctMode = 0;

				tmp = pCommand->Attribute( "Installed" );
				if ( tmp )  iInstalled = atoi( tmp );

				tmp = pCommand->Attribute( "Mode" );
				if ( tmp )  iMode = atoi( tmp );

				tmp = pCommand->Attribute( "MaxPercent" );
				if ( tmp )  iMaxPct = atoi( tmp );

				tmp = pCommand->Attribute( "CurrentInPctMode" );
				if ( tmp )  iCurrentInPctMode = atoi( tmp );

				iCmdRet = g_PoolCop.SetIonizerConfig( iInstalled,
													  iMode,
													  iMaxPct,
													  iCurrentInPctMode );
			}

			else if ( stricmp( pzCmdStr, "SetAutochlorConfig" ) == 0 )
			{
				int   iGasMode = 0,
					  iGasDuration = 0;
                float rSetpoint = 0;

				tmp = pCommand->Attribute( "GasMode" );
				if ( tmp )  iGasMode = atoi( tmp );

				tmp = pCommand->Attribute( "GasDurationPct" );
				if ( tmp )  iGasDuration = atoi( tmp );

                tmp = pCommand->Attribute( "SetPoint" );
                if ( tmp )  rSetpoint = atof( tmp );

				iCmdRet = g_PoolCop.SetAutochlorConfig( iGasMode,
											            iGasDuration,
                                                        (int)(rSetpoint * 10) );
			}

			else if ( stricmp( pzCmdStr, "SetWaterLevelConfig" ) == 0 )
			{
				int iInstalled = 0,
				    iAutoAdd = 0,
				    iAddContinuous = 0,
				    iMaxDuration = 0,
				    iAutoWaterReduce = 0,
				    iDrainingDuration = 0;

				tmp = pCommand->Attribute( "Installed" );
				if ( tmp )	iInstalled = atoi( tmp );

				tmp = pCommand->Attribute( "AutoAdd" );
				if ( tmp )  iAutoAdd = atoi( tmp );

				tmp = pCommand->Attribute( "Continuous" );
				if ( tmp )  iAddContinuous = atoi( tmp );

				tmp = pCommand->Attribute( "MaxFilling" );
				if ( tmp )  iMaxDuration = atoi( tmp );

				tmp = pCommand->Attribute( "AutoWaterReduce" );
				if ( tmp )  iAutoWaterReduce = atoi( tmp );

				tmp = pCommand->Attribute( "DrainingDuration" );
				if ( tmp )  iDrainingDuration = atoi( tmp );

				iCmdRet = g_PoolCop.SetWaterLevelControl( iInstalled,
														  iAutoAdd,
														  iAddContinuous,
														  iMaxDuration,
														  iAutoWaterReduce,
														  iDrainingDuration );
			}

			else if ( stricmp( pzCmdStr, "SetPumpConfig" ) == 0 )
			{
				/* <SetPumpConfig PumpType="n"
                 *                LowPressureAlarm="nnn"
				 *                ProtectionPressure="nnn"
				 *                ProtectOnLowPressure="n"
                 *                Cycle1Speed="n"
                 *                Cycle2Speed="n"
                 *                BackwashSpeed="n" />
				 */
				int iPressureLow = 0;
				int	iAlarmPressure = 0;
				int	iProtectPump = 0;
				int iPumpType = 0;
				int iSpeedCycle1 = 0;
				int iSpeedCycle2 = 0;
				int iSpeedBackwash = 0;

				tmp = pCommand->Attribute( "PumpType" );
				if ( tmp ) iPumpType = atoi( tmp );

				tmp = pCommand->Attribute( "LowPressureAlarm" );
				if ( tmp ) iPressureLow = atoi( tmp );

				tmp = pCommand->Attribute( "ProtectionPressure" );
				if ( tmp ) iAlarmPressure = atoi( tmp );

				tmp = pCommand->Attribute( "ProtectOnLowPressure" );
				if ( tmp ) iProtectPump = atoi( tmp );

				tmp = pCommand->Attribute( "Cycle1Speed" );
				if ( tmp ) iSpeedCycle1 = atoi( tmp );

				tmp = pCommand->Attribute( "Cycle2Speed" );
				if ( tmp ) iSpeedCycle2 = atoi( tmp );

				tmp = pCommand->Attribute( "BackwashSpeed" );
				if ( tmp ) iSpeedBackwash = atoi( tmp );

				iCmdRet = g_PoolCop.SetPumpConfig( iPressureLow,
												   iAlarmPressure,
												   iProtectPump,
												   iPumpType,
												   iSpeedCycle1,
												   iSpeedCycle2,
												   iSpeedBackwash );
			}

			else if ( stricmp( pzCmdStr, "SetPhConfig" ) == 0 )
			{
				/* <SetPhConfig Installed="n"
                 *              SetPoint="n.n"
                 *              Type="n"
                 *              MaxDosingTime="nn" />
				 */
				int   iInstalled = 0;
				float fSetpoint  = 0.0;
				int   iType      = 0;
				int   iMaxDosing = 0;

				tmp = pCommand->Attribute( "Installed" );
				if ( tmp ) iInstalled = atoi( tmp );

				tmp = pCommand->Attribute( "SetPoint" );
				if ( tmp ) fSetpoint = atof( tmp );

				tmp = pCommand->Attribute( "Type" );
				if ( tmp ) iType = atoi( tmp );

				tmp = pCommand->Attribute( "MaxDosingTime" );
				if ( tmp ) iMaxDosing = atoi( tmp );

				iCmdRet = g_PoolCop.SetPhConfig( iInstalled,
											     fSetpoint,
											     iType,
											     iMaxDosing );
			}

			else if ( stricmp( pzCmdStr, "ClearAlarm" ) == 0 )
			{
				/* <ClearAlarm Alarm="n" />
				 */
				int iAlarm = 0;

				tmp = pCommand->Attribute( "Alarm" );
				if ( tmp ) iAlarm = atoi( tmp );

				iCmdRet = g_PoolCop.ClearAlarm( iAlarm );
			}

			else if ( stricmp( pzCmdStr, "RestoreFactorySettings" ) == 0 )
			{
				/* <RestoreFactorySettings Confirm="yes|no" />
				 */
				const char *confirm = pCommand->Attribute( "Confirm" );
				iCmdRet = g_PoolCop.RestoreFactorySettings( confirm );
			}

			else if ( stricmp( pzCmdStr, "SetPoolConfig" ) == 0 )
			{
				/* <SetPoolConfig Volume="nnn"
                 *                FlowRate="nn"
                 *                Turnovers="nn"
                 *                FreezeProtection="nn"
                 *                CoverReductionPct="nn"
                 *                PoolType="n"
                 *                CoverSpeed="n"/>
				 */
				int iVolume = 0;
				int iFlowRate = 0;
				int iTurnovers = 0;
				int iFreezeProtection = 0;
				int iCoverReductionPct = 0;
				int iPoolType = 0;
				int iCoverSpeed = 0;

				tmp = pCommand->Attribute( "Volume" );
				if ( tmp ) iVolume = atoi( tmp );

				tmp = pCommand->Attribute( "FlowRate" );
				if ( tmp ) iFlowRate = atoi( tmp );

				tmp = pCommand->Attribute( "Turnovers" );
				if ( tmp ) iTurnovers = atoi( tmp );

				tmp = pCommand->Attribute( "FreezeProtection" );
				if ( tmp ) iFreezeProtection = atoi( tmp );

				tmp = pCommand->Attribute( "CoverReductionPct" );
				if ( tmp ) iCoverReductionPct = atoi( tmp );

				tmp = pCommand->Attribute( "PoolType" );
				if ( tmp ) iPoolType = atoi( tmp );

				tmp = pCommand->Attribute( "CoverSpeed" );
				if ( tmp ) iCoverSpeed = atoi( tmp );

				iCmdRet = g_PoolCop.SetPoolConfig( iVolume,
												   iFlowRate,
												   iTurnovers,
												   iFreezeProtection,
												   iCoverReductionPct,
												   iPoolType,
												   iCoverSpeed);
			}

			else if ( stricmp( pzCmdStr, "SetFilterTimers" ) == 0 )
			{
				/* <SetFilterTimers Timer1Enable="n"
				 *                  Timer2Enable="n"
				 *                  Timer2Mode="n"
                 *                  StartTime1="hh:mm"
                 *                  StopTime1="hh:mm"
                 *                  StartTime2="hh:mm"
                 *                  StopTime2="hh:mm" />
				 */
				int iTimer1Enable = 0,
				    iTimer2Enable = 0,
				    iTimer2Mode   = 0;

				tmp = pCommand->Attribute( "Timer1Enable" );
				if ( tmp ) iTimer1Enable = atoi( tmp );

				tmp = pCommand->Attribute( "Timer2Enable" );
				if ( tmp ) iTimer2Enable = atoi( tmp );

				tmp = pCommand->Attribute( "Timer2Mode" );
				if ( tmp ) iTimer2Mode = atoi( tmp );

				const char *pzStartTime1 = pCommand->Attribute( "StartTime1" );
				const char *pzStopTime1  = pCommand->Attribute( "StopTime1" );
				const char *pzStartTime2 = pCommand->Attribute( "StartTime2" );
				const char *pzStopTime2  = pCommand->Attribute( "StopTime2" );

				iCmdRet = g_PoolCop.SetFilterTimers( iTimer1Enable,
												     iTimer2Enable,
												     iTimer2Mode,
												     pzStartTime1,
												     pzStopTime1,
												     pzStartTime2,
												     pzStopTime2 );
			}

			else if ( stricmp( pzCmdStr, "SetFilterConfig" ) == 0 )
			{
				/* <SetFilterConfig BackwashDuration="www"
				 *                  RinseDuration="rrr"
				 *                  PressureHigh="ppp"
				 *                  AutoPressure="yes/no"
				 *                  MaxDaysBetweenBackwash="nnn",
				 *                  WasteLineValve="n" />
                 * with www, rrr, ppp numerical values, autoweekly for "None, monday, tuesday....."
				 */
				int iBackwashDuration = 0,
					iRinseDuration = 0,
					iPressureHigh = 0,
					iAutoBackwash = 0,
					iMaxDaysBetweenBackwash = 0,
					iWasteLineValve = 0;

				tmp = pCommand->Attribute( "PressureHigh" );
				if ( tmp ) iPressureHigh = atoi( tmp );

				tmp = pCommand->Attribute( "BackwashDuration" );
				if ( tmp ) iBackwashDuration = atoi( tmp );

				tmp = pCommand->Attribute( "RinseDuration" );
				if ( tmp ) iRinseDuration = atoi( tmp );

				tmp = pCommand->Attribute( "AutoPressure" );
				if ( tmp ) iAutoBackwash = atoi( tmp );

				tmp = pCommand->Attribute( "MaxDaysBetweenBackwash" );
				if ( tmp ) iMaxDaysBetweenBackwash = atoi( tmp );

				tmp = pCommand->Attribute( "WasteLineValve" );
				if ( tmp ) iWasteLineValve = atoi( tmp );

				iCmdRet = g_PoolCop.SetFilterParameters( iPressureHigh,
						                                 iBackwashDuration,
													     iRinseDuration,
													     iAutoBackwash,
													     iMaxDaysBetweenBackwash,
													     iWasteLineValve );
			}

			else if ( stricmp( pzCmdStr, "SetAuxTimer" ) == 0 )
			{
				/* <SetAuxTimer Aux="n?
                 *              TimerSlave="n"
                 *              TimerStatus="n"
                 *              StartTime="hh:mm"
                 *              StopTime="hh:mm"
                 *              WeekDays="0101100" />
				 */
				int	iAux = 0,
				    iSlave = 0,
				    iStatus = 0;
				int iRunMonday    = 0,
				    iRunTuesday   = 0,
				    iRunWednesday = 0,
				    iRunThursday  = 0,
				    iRunFriday    = 0,
				    iRunSaturday  = 0,
				    iRunSunday    = 0;

				tmp = pCommand->Attribute( "Aux" );
				if ( tmp ) iAux = atoi( tmp );

				tmp = pCommand->Attribute( "TimerSlave" );
				if ( tmp ) iSlave = atoi( tmp );

				tmp = pCommand->Attribute( "TimerStatus" );
				if ( tmp ) iStatus = atoi( tmp );

				tmp = pCommand->Attribute( "WeekDays" );
				if ( tmp )
				{
					iRunMonday    = (tmp[0] == '1');
					iRunTuesday   = (tmp[1] == '1');
					iRunWednesday = (tmp[2] == '1');
					iRunThursday  = (tmp[3] == '1');
					iRunFriday    = (tmp[4] == '1');
					iRunSaturday  = (tmp[5] == '1');
					iRunSunday    = (tmp[6] == '1');
				}

				const char *pzStarTime = pCommand->Attribute( "StartTime" );
			    const char *pzStopTime = pCommand->Attribute( "StopTime" );

				iCmdRet = g_PoolCop.SetAuxTimer( iAux,
						                         iSlave,
						                         iStatus,
						                         pzStarTime,
											     pzStopTime,
											     iRunMonday,
											     iRunTuesday,
											     iRunWednesday,
											     iRunThursday,
											     iRunFriday,
											     iRunSaturday,
											     iRunSunday );
			}

			else if ( stricmp( pzCmdStr, "SetAuxLabels" ) == 0 )
			{
				/* <SetAuxLabels Aux1="nn"
                 *               Aux2="nn"
                 *               Aux3="nn"
                 *               Aux4="nn"
                 *               Aux5="nn"
                 *               Aux6="nn" />
				 */
				int iAux[6] = { 0 };

				for ( int i = 0; i < 6; i++ )
				{
					char szName[6];
					sprintf( szName, "Aux%d", i+1 );
					tmp = pCommand->Attribute( szName );
					if ( tmp ) iAux[i] = atoi( tmp );
				}
				iCmdRet = g_PoolCop.SetAuxLabels( iAux[0],
			 							          iAux[1],
										          iAux[2],
										          iAux[3],
										          iAux[4],
										          iAux[5] );
			}


			else if ( stricmp( pzCmdStr, "SetInputConfig" ) == 0 )
			{
				/* <SetInputConfig Input1Dir="n"
				 *                 Input1Func="m"
				 *                 Input2Dir="n"
				 *                 Input2Func="m" />
				 */
				int iInput1Dir  = 0,
				    iInput1Func = 0,
				    iInput2Dir  = 0,
				    iInput2Func = 0;

				tmp = pCommand->Attribute( "Input1Dir" );
				if ( tmp )	iInput1Dir = atoi( tmp );

				tmp = pCommand->Attribute( "Input1Func" );
				if ( tmp )	iInput1Func = atoi( tmp );

				tmp = pCommand->Attribute( "Input2Dir" );
				if ( tmp )	iInput2Dir = atoi ( tmp );

				tmp = pCommand->Attribute( "Input2Func" );
				if ( tmp )	iInput2Func = atoi( tmp );

				iCmdRet = g_PoolCop.SetInputConfig( iInput1Dir,
													iInput1Func,
						                            iInput2Dir,
						                            iInput2Func );
			} /* else if */

            else if ( stricmp( pzCmdStr, "FirmwareUpdate" ) == 0 )
            {
                /* <FirmwareUpdate FTPServer="ftp.poolcopilot.com"
                 *                 Username="poolcopilot"
                 *                 Password="password"
                 *                 FileName="Firmware/PoolCopilot_APP.s19"
                 *                 SHA1="XXXXXXXXXXXXXXXXXXXXXXXXXX"
                 *                 TestMode="n"
                 *                 SupressReboot="n" />
                 */
                const char *pzUrl      = pCommand->Attribute( "FTPServer" ); // must be present
                const char *pzUserName = pCommand->Attribute( "Username" );  // must be present
                const char *pzPassword = pCommand->Attribute( "Password" );  // must be present
                const char *pzFilename = pCommand->Attribute( "Filename" );  // must be present
                const char *pzSha1     = pCommand->Attribute( "SHA1" );      // can be NULL
                if ( (pzUrl      != NULL) &&
                     (pzUserName != NULL) &&
                     (pzPassword != NULL) &&
                     (pzFilename != NULL) )
                {
                    tmp = pCommand->Attribute( "TestMode" );
                    bool fTestMode = ( tmp != NULL && *tmp == 'y' );
                    tmp = pCommand->Attribute( "SuppressReboot" );
                    bool fSuppress = ( tmp != NULL && *tmp == 'y' );

                    FirmwareUpdate FwUpd;
                    FwUpd.SetUrl( pzUrl );
	                FwUpd.SetCredentials( pzUserName, pzPassword );
	                FwUpd.DoUpdateFile( pzFilename, pzSha1, fTestMode, fSuppress );
                } /* if */
            } /* else if */

            else if ( stricmp( pzCmdStr, "HttpFirmwareUpdate" ) == 0 )
            {
                /* <HttpFirmwareUpdate URL="http://www.poolcopilot.com/firmware/PoolCopilot_APP_SB700EX.s19"
                 *                     SHA1="XXXXXXXXXXXXXXXXXXXXXXXXXX"
                 *                     TestMode="y/n"
                 *                     SupressReboot="y/n" />
                 */
            	const char *pzUrl      = pCommand->Attribute( "URL" ); 	// must be present
                const char *pzSha1     = pCommand->Attribute( "SHA1" ); // can be NULL
                if (pzUrl != NULL)
                {
                	tmp = pCommand->Attribute( "TestMode" );
                	bool fTestMode = ( tmp != NULL && tolower(*tmp) == 'y' );
                	tmp = pCommand->Attribute( "SuppressReboot" );
                	bool fSuppress = ( tmp != NULL && tolower(*tmp) == 'y' );

                	Poco::URI url;
                	url = pzUrl;
                	const char *pzFilename = url.getPath().c_str();

                	FirmwareUpdate FwUpd;
                	FwUpd.SetUrl( pzUrl );
                	FwUpd.DoUpdateFile( pzFilename, pzSha1, fTestMode, fSuppress );
                } /* if */
            } /* else if */

#ifdef _DEBUG
            else if ( stricmp( pzCmdStr, "fwupd" ) == 0 )
            {
            	FirmwareUpdate	fwupd;
            	fwupd.SetUrl( "ftp://ftp.thehoys.com/firmware" );
            	fwupd.SetCredentials( "poolcop", "firmware" );
#ifdef SB72
            	char *pszFileName = "PoolCopilot_APP.s19";
#else
          		char *pszFileName = "PoolCopilot_SB700EX_APP.s19";
#endif
           		fwupd.DoUpdateFile( pszFileName, NULL, false, false );
            } /* else if */
#endif

            else if ( stricmp( pzCmdStr, "SetConfig" ) == 0 )
            {
            	/* <SetConfig ServerUrl="http://poolcopilot.level141.com/index.php"
				 *            AltServerUrl="http://192.168.2.116/index.php"
				 *            ConnectionInterval="60"
                 *            WatchdogEnable="n" />
				 */
                tmp = pCommand->Attribute( "ServerUrl" );
				if ( tmp )	g_ConfigData.SetValue( "ServerUrl", tmp );

				tmp = pCommand->Attribute( "AltServerUrl" );
				if ( tmp )	g_ConfigData.SetValue( "AltServerUrl", tmp );

				tmp = pCommand->Attribute( "ConnectionInterval" );
				if ( tmp )	g_ConfigData.SetValue( "ConnectionInterval", atoi(tmp) );

				tmp = pCommand->Attribute( "WatchdogEnable" );
				if ( tmp )	g_ConfigData.SetValue( "WatchdogEnabled", atoi(tmp) );

				g_ConfigData.Save();
            } /* else if */

            else if ( stricmp( pzCmdStr, "RequestData" ) == 0 )
			{
            	int iDataType = 0,
            	    iAux      = 1;

				/* <RequestData DataType="n"
				 *              Aux="m" />
				 */
				tmp = pCommand->Attribute( "DataType" );
				if ( tmp )	iDataType = atoi( tmp );

				tmp = pCommand->Attribute( "Aux" );
				if ( tmp )	iAux = atoi( tmp );

				iCmdRet = g_PoolCop.RequestData( iDataType, iAux );
			} /* else if */

            else if ( stricmp( pzCmdStr, "SetForcedPumpMode" ) == 0 )
            {
            	int iPumpMode = 0;
            	/* <SetForcedPumpMode Mode="n" />
            	 */
            	tmp = pCommand->Attribute( "Mode" );
            	if ( tmp ) iPumpMode = atoi( tmp );

            	iCmdRet = g_PoolCop.SetForcedPumpMode( iPumpMode );
            } /* else if */

			else if ( stricmp( pzCmdStr, "SetPumpSpeed" ) == 0 )
            {
            	int iSpeed = 0;
            	/* <SetPumpSpeed Speed="n" />
            	 */
            	tmp = pCommand->Attribute( "Speed" );
            	if ( tmp ) iSpeed = atoi( tmp );

            	iCmdRet = g_PoolCop.SetPumpSpeed( iSpeed );
            } /* else if */

			else if (stricmp( pzCmdStr, "SetServiceMode" ) == 0 )
			{
				int iServiceMode = 0;
				int iAirAntifreezeTrigger = 0;
				int iWaterTempShift = 0;

				/* <SetServiceMode ServiceMode="n"
				 *                 AirAntifreezeTrigger="n"
				 *                 WaterTempShift="n" />
				 */

				tmp = pCommand->Attribute( "ServiceMode" );
				if ( tmp ) iServiceMode = atoi( tmp );

				tmp = pCommand->Attribute( "AirAntifreezeTrigger" );
				if ( tmp ) iAirAntifreezeTrigger = atoi( tmp );

				tmp = pCommand->Attribute( "WaterTempShift" );
				if ( tmp ) iWaterTempShift = atoi( tmp );

				iCmdRet = g_PoolCop.SetServiceMode( iServiceMode,
						                            iAirAntifreezeTrigger,
						                            iWaterTempShift );
			}

			else if (stricmp( pzCmdStr, "ExternalResetRequest" ) == 0 )
			{
				/* <ResetPoolCop/>
				 */
				iCmdRet = g_PoolCop.ResetPoolCop();
			}

			else if (stricmp( pzCmdStr, "pHAutoCalibrate" ) == 0 )
			{
				int iCalibrationValue = 0;

				/* <pHAutoCalibrate CalibrationValue="n" />
				 */
				tmp = pCommand->Attribute( "CalibrationValue" );
				if ( tmp ) iCalibrationValue = atoi( tmp );

				iCmdRet = g_PoolCop.pHAutoCalibration( iCalibrationValue );
			}

#endif
 			if ( m_pResultsXml )
 			{
 				m_pResultsXml->SetAttribute( pzCmdStr, iCmdRet );
 			} /* if */

 			iCommandCnt++;
            pCommand = pCommand->NextSiblingElement();
        } /* while */


        // See if there's a next-connection interval sent from the server
        //
#if 0
        tmp = pCommands->Attribute( "NextConnect" );
        if ( tmp ) m_iNextConnect = atoi( tmp );
#endif
    } /* if */

    return iCommandCnt;

} /* PoolCopilot::ProcessXmlResponse() */


int PoolCopilot::ParseControlPump( TiXmlElement *pCommand )
{
	int iState = 0;

	const char *tmp = pCommand->Attribute( "State" );
	if ( tmp ) iState = atoi( tmp );

	int iRet = g_PoolCop.ControlPump( iState );
	return iRet;
}


int PoolCopilot::ParseSetTime( TiXmlElement *pCommand )
{
	const char *tmp = pCommand->Attribute( "Date" );
	int iRet = g_PoolCop.SetDate( tmp );
	if ( iRet == 0 )
	{
		tmp = pCommand->Attribute( "Time" );
		iRet = g_PoolCop.SetTime( tmp );
	}
	return iRet;
}
