/*
 * Watchdog.h
 *
 *  Created on: Dec 3, 2011
 *      Author: david
 */

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#define WATCHDOG_MAX_TIME	16383

class Watchdog
{
public:
	Watchdog();
	virtual ~Watchdog();

	void 		 Start( unsigned int MaxTime_ms );
	void 		 Stop( void );
	bool 		 IsResetDueToWD( void );
	void 		 Kick( void );
	unsigned int TimeLeft( void );
	void         TestMode( bool fTestMode );

	static void  TaskThreadWrapper( void* pArgs );
	void         TaskThread( void );

	static int   CmdWatchdog( int argc, char *argv[], void *pContext );

private:
	bool		 m_fEnabled;
	bool		 m_fTestMode;
	unsigned int m_uTimeout;
	unsigned int m_uShortestTimeLeft;
	bool		 m_fLastResetDueToWD;
	bool	     m_fThreadRunning;
	bool		 m_fShutdownThread;
};

extern Watchdog g_Watchdog;

#endif /* WATCHDOG_H_ */
