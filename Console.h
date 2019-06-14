/*
 * Console.h
 *
 *  Created on: Jul 20, 2011
 *      Author: david
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

typedef int (*PFN_CMDFUNC)( int argc, char *argv[], void *pContext );


#define MOD_MAIN		0
#define MOD_POOLCOPILOT	1
#define MOD_SERVER		2
#define MOD_POOLCOP   	3
#define MOD_FWUPDATE	4
#define MOD_WATCHDOG	5



class CConsole
{
public:
	CConsole();
	~CConsole();

	void Initialize( int fdSerial );

	int  AddCommand( char *pszCmd, char *pszHelpText, PFN_CMDFUNC pfnCmd, void *pContext );

	void DebugTrace( int iModule, int iLevel, const char *pszFmt, ... );
	void DebugTraceStr( int iModule, int iLevel, const char *pszStr );
	int  SetDebugLevel( int iModule, int iLevel );
	int  GetDebugLevel( int iModule );

	void Printf( char *pszFmt, ... );

	static int CmdDisplayHelp( int argc, char *argv[], void *pContext );
	static int CmdLogout( int argc, char *argv[], void *pContext );
	static int CmdPump( int argc, char *argv[], void *pContext );
	static int CmdAux( int argc, char *argv[], void *pContext );
	static int CmdDebug( int argc, char *argv[], void *pContext );
	static int CmdIpconfig( int argc, char *argv[], void *pContext );
	static int CmdReboot( int argc, char *argv[], void *pContext );
	static int CmdShowMemory( int argc, char *argv[], void *pContext );
    static int CmdSha1Test( int argc, char *argv[], void *pContext );
    static int CmdUpdateFirmware( int argc, char *argv[], void *pContext );
    static int CmdHttpUpdate( int argc, char *argv[], void *pContext );
    static int CmdVer( int argc, char *argv[], void *pContext );
    static int CmdPing( int argc, char *argv[], void *pContext );
    static int CmdDate( int argc, char *argv[], void *pContext );
    static int CmdTime( int argc, char *argv[], void *pContext );

	static int   ProcessAuth( const char *name, const char *pass );
	static int   ProcessCommand( const char *command, FILE *fp, void *pData );
	static void *ProcessConnect( FILE *fp );
	static void  ProcessPrompt( FILE *fp, void *pData );
	static void  ProcessDisconnect( FILE *fp, int cause, void *pData );

private:


};


extern CConsole	Console;

#endif /* CONSOLE_H_ */
