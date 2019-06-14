/*
 * ConfigData.cpp
 *
 *  Created on: Jul 18, 2011
 *      Author: david
 */


#include <stdio.h>
#include <utils.h>
#include "ConfigData.h"
#include "Console.h"
#include "Poco/URI.h"
#include <string.h>
#include "Utilities.h"
#include "Sha1.h"


#define CONFIG_SIGNATURE    0xBEEFDEAD
#define CONFIG_BLOCK_SIZE   (8*1024)
#define CONFIG_SHA1_LENGTH  41
#define CONFIG_XML_MAX      (CONFIG_BLOCKSIZE - sizeof(DWORD) - CONFIG_SHA1_LENGTH )

typedef struct
{
    DWORD   dwSignature;
    char    szSha1[CONFIG_SHA1_LENGTH]; // SHA1 of the XML blob
    char    szXmlStr[1];                // Variable length XML blob
} CONFIG_BLOCK, *PCONFIG_BLOCK;



ConfigData::ConfigData() :
	m_fInitialized( false ),
	m_fDataValid( false )
{

}

ConfigData::~ConfigData()
{
	if ( m_fDataChanged )
	{
		Save();
	}
}


void ConfigData::Initialize( void )
{
	PCONFIG_BLOCK pConfig = (PCONFIG_BLOCK)GetUserParameters();
	if ( pConfig != NULL )
	{
		if ( pConfig->dwSignature == CONFIG_SIGNATURE )
		{
            // Calculate SHA1 on existing XML string
            //
            CSHA1   sha1;
            sha1.Update( (unsigned char *)pConfig->szXmlStr, strlen(pConfig->szXmlStr));
	        sha1.Final();
	        TCHAR   szSha1[41];
	        sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );

            // Compare calculated SHA1 to the one in the header
            //
            if ( strcmp( szSha1, pConfig->szSha1 ) == 0 )
            {
                // Load and parse the XML string
                //
                m_pDoc = new TiXmlDocument();
                m_pDoc->Parse( pConfig->szXmlStr, 0, TIXML_DEFAULT_ENCODING );

                TiXmlHandle docHandle( m_pDoc );
                m_pElement = docHandle.FirstChild( "PoolCopilotConfig" ).ToElement();
                if ( m_pElement )
                {
                	m_fDataValid   = true;
                	m_fDataChanged = false;
                	m_fInitialized = true;
                }
		    }
		}

		if ( !m_fDataValid )
		{
			WriteDefaults();
			Save();
			m_fDataValid   = false;
			m_fDataChanged = false;
			m_fInitialized = true;
		}
	}

	Console.AddCommand( "set", "View/set configuration parameters", CmdSet, this );
}

void ConfigData::Save( void )
{
    Lock();

    // Convert XML document to a string
    //
    TiXmlPrinter printer;
	printer.SetIndent( "" );
	printer.SetLineBreak( "" );
	m_pDoc->Accept( &printer );
	const char *pszXmlStr = printer.CStr();

    // Calculate the SHA1 on the string
    //
    CSHA1   sha1;
	sha1.Update( (unsigned char *)pszXmlStr, strlen(pszXmlStr));
	sha1.Final();
	TCHAR   szSha1[41];
	sha1.ReportHash( (TCHAR*)szSha1, CSHA1::REPORT_HEX_SHORT );

    // Build the output block
    //
    DWORD dwSize = sizeof(CONFIG_BLOCK) + strlen(pszXmlStr) + 1;
    PCONFIG_BLOCK pConfig = (PCONFIG_BLOCK)malloc( dwSize );
    if ( pConfig )
    {
        memset( pConfig, 0, dwSize );
        pConfig->dwSignature = CONFIG_SIGNATURE;
        strcpy( pConfig->szSha1, szSha1 );
        strcpy( pConfig->szXmlStr, pszXmlStr );

        SaveUserParameters( pConfig, dwSize );
        m_fDataValid = true;
	    m_fDataChanged = false;
        free( pConfig );
    }

    Unlock();
}


int ConfigData::CmdSet( int argc, char *argv[], void *pContext )
{
	ConfigData* pThis = (ConfigData*)pContext;

	if ( argc == 1 )
	{
		Console.Printf( "\r\n" );
		Console.Printf( "Configuration Settings:\r\n" );
		Console.Printf( "         ServerUrl: %s\r\n",   pThis->GetStringValue( "ServerUrl" ) );
		Console.Printf( "      AltServerUrl: %s\r\n",   pThis->GetStringValue( "AltServerUrl" ) );
		Console.Printf( "   WatchdogEnabled: %s\r\n",   pThis->GetIntValue( "WatchdogEnabled" ) ? "yes" : "no" );
		Console.Printf( "      ResetCounter: %d\r\n",   pThis->GetIntValue( "ResetCounter" ) );

		return 0;
	}

	if ( stricmp( argv[1], "defaults" ) == 0 )
	{
		pThis->WriteDefaults();
		pThis->Save();
		Console.Printf( "\r\nResetting to defaults - reboot to take effect\r\n" );
	}
	if ( stricmp( argv[1], "ServerUrl" ) == 0 )
	{
		if ( argc == 3 )
		{
			Console.Printf( "\r\nSetting ServerUrl to \"%s\"\r\n", argv[2] );
			pThis->SetValue( "ServerUrl", argv[2] );
			pThis->Save();
            pThis->UrlsHaveChanged( true );
		}
	}
	else if ( stricmp( argv[1], "AltServerUrl" ) == 0 )
	{
		if ( argc == 3 )
		{
			Console.Printf( "\r\nSetting AltServerUrl to \"%s\"\r\n", argv[2] );
			pThis->SetValue( "AltServerUrl", argv[2] );
			pThis->Save();
            pThis->UrlsHaveChanged( true );
		}
	}
	else if ( stricmp( argv[1], "WatchdogEnabled" ) == 0 )
	{
		if ( argc == 3 )
		{
			bool fWatchdogEnabled = ( (*argv[2] == 'y') || (*argv[2] == 'Y') );
			Console.Printf( "\r\nSetting WatchdogEnabled to \"%s\"\r\n", fWatchdogEnabled ? "yes" : "no" );
            pThis->SetValue( "WatchdogEnabled", fWatchdogEnabled );
			pThis->Save();
		}
	}

	return 0;
}


void ConfigData::WriteDefaults( void )
{
	Lock();

	if ( m_pDoc )
		delete m_pDoc;

	m_pDoc = new TiXmlDocument();
	if ( m_pDoc )
	{
		TiXmlDeclaration *pDecl = new TiXmlDeclaration( "1.0", "UTF-8", "yes" );
		m_pDoc->LinkEndChild( pDecl );
	    m_pElement = new TiXmlElement( "PoolCopilotConfig" );
	}
	if ( m_pElement )
    {
		m_pDoc->LinkEndChild( m_pElement );
	    m_pElement->SetAttribute( "ServerUrl",          DEFAULT_SERVER_URL );
	    m_pElement->SetAttribute( "AltServerUrl",       DEFAULT_ALT_SERVER_URL );
	    m_pElement->SetAttribute( "ResetCounter",       0 );
	    m_pElement->SetAttribute( "WatchdogEnabled",    0 );
	    m_pElement->SetAttribute( "WatchdogInterval",   0 );
	    m_pElement->SetAttribute( "IPAddress",          0 );
	    m_pElement->SetAttribute( "SubnetMask",         0 );
	    m_pElement->SetAttribute( "GatewayAddress",     0 );
	    m_pElement->SetAttribute( "DnsAddress",         0 );
    }

	Unlock();

} /* ConfigData::WriteDefaults() */


