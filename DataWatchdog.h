/*
 * DataWatchdog.h
 *
 *  Created on: Jan 29, 2012
 *      Author: david
 */

#ifndef DATAWATCHDOG_H_
#define DATAWATCHDOG_H_

#include <predef.h>
#include <startnet.h>
#include "CritSec.h"


class DataWatchdog
{
public:
	DataWatchdog();
	virtual ~DataWatchdog();

	void 		Start( void );
	static void TaskThreadWrapper( void* pArgs );
	void        TaskThread( void );

    void 		Lock()             	{ m_CritSec.Lock(); }
    void 		Unlock()           	{ m_CritSec.Unlock(); }

	void		StaticDataRcvd()	{ Lock(); m_dwLastStaticData  = TimeTick; Unlock(); }
    void		DynamicDataRcvd()	{ Lock(); m_dwLastDynamicData = TimeTick; Unlock(); }

	DWORD		GetStaticDataRcvd()	{ Lock(); DWORD ret = m_dwLastStaticData;  Unlock(); return ret; }
    DWORD  		GetDynamicDataRcvd(){ Lock(); DWORD ret = m_dwLastDynamicData; Unlock(); return ret; }

private:
	bool		m_fThreadRunning;
	bool		m_fShutdownThread;

	CritSec		m_CritSec;

	DWORD		m_dwLastStaticData;
	DWORD		m_dwStaticDataTimeout;
	DWORD		m_dwLastDynamicData;
	DWORD		m_dwDynamicDataTimeout;
};

extern DataWatchdog g_DataWatchdog;

#endif /* DATAWATCHDOG_H_ */
