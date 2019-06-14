/*
 * PoolCopilot.h
 *
 *  Created on: Jul 2, 2011
 *      Author: david
 */

#ifndef POOLCOPILOT_H_
#define POOLCOPILOT_H_


#include "ServerInterface.h"
#include "PoolCopDataBindings.h"
#include "TinyXml/TinyXml.h"

#define SERVER_BACKOFF_NORMAL		2
#define SERVER_BACKOFF_ERROR		30

class PoolCopilot
{
public:
	PoolCopilot();
	~PoolCopilot();

	int			Startup( void );
	int			Shutdown( void );

	static void TaskThreadWrapper( void* pArgs );
	void 		TaskThread( void );

	int 		ConnectToServer( void );
	int			SendDataToServer( DWORD dwFlags );
	int  		ProcessXmlResponse( const char *pszXmlStr );
	int			DisconnectFromServer( void );

	void		AsyncDataRcvd( void );

private:
	bool			m_fThreadRunning;
	bool			m_fShutdownThread;

	ServerInterface	*m_pServer;

	bool			m_fUseAltServer;
	int				m_iHttpKeepAlive;
	int 			m_iServerBackoff;

	TiXmlElement*   m_pResultsXml;

public:
	static int 		ParseSetTime    ( TiXmlElement *pCommand );
	static int 		ParseControlPump( TiXmlElement *pCommand );
};

extern PoolCopilot g_PoolCopilot;

#endif /* POOLCOPILOT_H_ */
