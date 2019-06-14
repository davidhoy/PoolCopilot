/*
 * ConfigData.h
 *
 *  Created on: Jul 18, 2011
 *      Author: david
 */

#ifndef CONFIGDATA_H_
#define CONFIGDATA_H_

#include <system.h>
#include <basictypes.h>
#include <string>
#include "Tinyxml/TinyXML.h"
#include "CritSec.h"


#define DEFAULT_SERVER_URL					"http://bridge.poolcopilot.com/"
#define DEFAULT_ALT_SERVER_URL				"http://192.168.2.116/index.php"
#define DEFAULT_IP_ADDRESS					0
#define DEFAULT_SUBNET_MASK					0
#define DEFAULT_GW_ADDRESS					0
#define DEFAULT_DNS_ADDRESS					0


class ConfigData
{
public:
	ConfigData();
	~ConfigData();

	void Initialize( void );
    void WriteDefaults( void );
	void Save( void );

	static int CmdSet( int argc, char *argv[], void *pContext );

	int GetIntValue( const char *name )
	{
        Lock();
		if ( !m_fInitialized )
			Initialize();

		int ret = 0;
		if ( m_pElement )
		{
			const char *tmp = m_pElement->Attribute( name );
			if ( tmp )
				ret = atoi( tmp );
		}
        Unlock();
		return ret;
	}

	float GetFloatValue( const char *name )
	{
        Lock();
		if ( !m_fInitialized )
			Initialize();

		float ret = 0;
		if ( m_pElement )
		{
			const char *tmp = m_pElement->Attribute( name );
			if ( tmp )
				ret = atof( tmp );
		}
        Unlock();
		return ret;
	}

	const char* GetStringValue( const char *name )
	{
        Lock();
		if ( !m_fInitialized )
			Initialize();

		const char* ret = "";
		if ( m_pElement )
		{
			const char *tmp = m_pElement->Attribute( name );
			if ( tmp )
				ret = tmp;
		}
        Unlock();
		return ret;
	}

	template <class T> void SetValue( const char *name, const T value )
	{
        Lock();
		if ( m_pElement )
		{
			m_pElement->SetAttribute( name, value );
            m_fDataChanged = true;
		}
        Unlock();
	}

    void Lock()     { m_CritSec.Lock(); }
    void Unlock()   { m_CritSec.Unlock(); }

    void UrlsHaveChanged( bool flag )   { m_fUrlsChanged = flag; }
    bool HaveUrlsChanged()              { return m_fUrlsChanged; }


private:
	bool			m_fInitialized;
	bool			m_fDataValid;
	bool			m_fDataChanged;
    bool            m_fUrlsChanged;
    CritSec         m_CritSec;

	TiXmlDocument	*m_pDoc;
	TiXmlElement    *m_pElement;

};

extern ConfigData g_ConfigData;

#endif /* CONFIGDATA_H_ */


