/*
 * ServerInterface.cpp
 *
 *  Created on: Jun 22, 2011
 *      Author: david
 */

#include "ServerInterface.h"
#include "Console.h"
#include "Utilities.h"


#define DebugTrace( level, fmt, ... )		Console.DebugTrace( MOD_SERVER, level, fmt, ##__VA_ARGS__ )
#define DebugTraceStr( level, str )			Console.DebugTraceStr( MOD_SERVER, level, str )


#define HTTP_READ_TIMEOUT			(10 * TICKS_PER_SECOND)
#define DNS_LOOKUP_TIMEOUT			(5 * TICKS_PER_SECOND)
#define DEFAULT_POOLCOPILOT_IP		"40.69.207.195"


char *timestamp(void)
{
	static char timestr[40];

	time_t t = time(NULL);
	struct tm now = *gmtime(&t);
	sprintf(timestr, "%04d/%02d/%02d %02d:%02d:%02d",
			now.tm_year+1900,
			now.tm_mon+1,
			now.tm_mday,
			now.tm_hour,
			now.tm_min,
			now.tm_sec);
	return timestr;
}


/********************************************************************
* ServerInterface::ServerInterface()
*
* Default class constructor
*
* Parameters:   N/A
*
* Return:       N/A
*********************************************************************/
ServerInterface::ServerInterface() :
	m_fdServer( 0 ),
	m_iTimeout( HTTP_READ_TIMEOUT ),
	m_bAbortFlag( FALSE )
{
	m_strContentType = "text/xml";
	m_ServerIp = 0;

} /* ServerInterface::ServerInterface() */


/********************************************************************
* ServerInterface::~ServerInterface()
*
* Default class destructor
*
* Parameters:   N/A
*
* Return:       N/A
*********************************************************************/
ServerInterface::~ServerInterface()
{
	Close();

} /* ServerInterface::~ServerInterface() */


/********************************************************************
* ServerInterface::Connect()
*
* This method established a TCP connection to the specified URL.
*
* Parameters:   pszUrl - URL to be connected to
*
* Return:       int -
*********************************************************************/
int ServerInterface::Connect( const char *pszUrl )
{
	int		iRet = 0;

	// Parse and save a copy of the URL
    //
	m_Url = pszUrl;


	// Use DNS to resolve IP address of the server
    //
	if ( m_ServerIp == 0 )
	{
		DebugTrace( 2, "\r\n%s: Using DNS to get IP address of %s...",
				    timestamp(), m_Url.getHost().c_str() );
		int status = GetHostByName( m_Url.getHost().c_str(),
									&m_ServerIp,
									EthernetDNS,
									DNS_LOOKUP_TIMEOUT );
		if ( status != DNS_OK )
		{
			if ( status == DNS_TIMEOUT )
				DebugTrace( 2, "DNS Error: TIMEOUT" );
			else if ( status == DNS_NOSUCHNAME )
				DebugTrace( 2, "DNS Error: NO SUCH NAME" );
			else
				DebugTrace( 2, "DNS Error: %d", status );

			m_ServerIp = StrToIP(DEFAULT_POOLCOPILOT_IP);
			DebugTrace( 2, "Falling back to hard-coded IP address %s", m_ServerIp);
			//iRet = status;
		} /* if */
		else
		{
			DebugTrace( 2, "%s", IPToStr( m_ServerIp ) );
		} /* else */
	} /* if */


	// Connect to the server.
	//
	DebugTrace( 2, "\r\n%s: Connecting to %s [%s]...",
			    timestamp(), m_Url.getHost().c_str(), IPToStr(m_ServerIp) );
	Close();
	m_fdServer = connect( m_ServerIp, 0, m_Url.getPort(), m_iTimeout );
	if ( m_fdServer < 0 )
	{
		DebugTrace( 2, "error, connection failed" );
		m_ServerIp = 0; 	// Force DNS resolution next time
		iRet = -1;
	} /* if */
	else
	{
		DebugTrace( 2, "connected" );
	} /* else */

	return iRet;

} /* ServerInterface::Connect() */


/********************************************************************
* ServerInterface::Close()
*
* Close the open TCP connection
*
* Parameters:   void
*
* Return:       int - 0 for success
*********************************************************************/
int ServerInterface::Close( void )
{
	if ( m_fdServer > 0 )
	{
		close( m_fdServer );
		m_fdServer = 0;
		m_ServerIp = 0; 	// Force DNS resolution next time
		m_Url.clear();
	} /* if */
	return 0;

} /* ServerInterface::Close() */



/********************************************************************
* ServerInterface::HttpPost()
*
* Send an HTTP POST to the open TCP port, and receive the corresponding
* reply.  This method supports long-polling with a caller-defined
* timeout.
*
* Parameters:   pszExtraHeader  - any extra stuff for HTTP header
*               iKeepAlive      - keep-alive value for HTTP header
*               pszPostData     - data for body of POST
*               cbPostData      - size of data for POST
*               pszPostResponse - buffer for POST response
*               pcbPostResponse - input is size of response buffer,
*                                 output is size of received response
*               iHttpTimeout    - timeout for HTTP response, in seconds
*
* Return:       int - 0 for success, else error code
*********************************************************************/
int ServerInterface::HttpPost( const char *pszExtraHeaders,
							   const int  iKeepAlive,
							   const char *pszPostData,
							   const int  cbPostData,
							   char       *pszPostResponse,
							   int        *pcbPostResponse,
							   const int  iHttpTimeout )
{
    int     iRet = -1;


    // If we have no open connection return error
	if ( m_fdServer <= 0 )
	{
		return -1;
	} /* if */


	// Write HTTP POST header
	//
	DebugTrace( 3, "\r\n%s: Sending HTTP post to server...", timestamp());

	char szBuffer[80];
	const char *path = m_Url.getPath().c_str();
	if (strlen(path) == 0)
	{
		path = "/";
	}
	sprintf( szBuffer, "POST %s HTTP/1.1\r\n", path );
	writestring( m_fdServer, szBuffer );
	sprintf( szBuffer, "Host: %s\r\n", m_Url.getHost().c_str() );
	writestring(  m_fdServer, szBuffer );
	writestring( m_fdServer, "User-Agent: Netburner/SB72\r\n" );
	sprintf( szBuffer, "Content-Length: %d\r\n", cbPostData );
	writestring( m_fdServer, szBuffer );
	writestring( m_fdServer, "Accept: text/xml\r\n" );
	writestring( m_fdServer, "Content-Type: text/xml\r\n" );
	if ( iKeepAlive > 0 )
	{
		sprintf( szBuffer, "Keep-Alive: %d\r\n", iKeepAlive );
		writestring( m_fdServer, szBuffer );
		writestring( m_fdServer, "Connection: keep-alive\r\n" );
	} /* if */
	else
	{
		writestring( m_fdServer, "Connection: close\r\n" );
	} /* else */
	if ( pszExtraHeaders != NULL && strlen( pszExtraHeaders ) > 0 )
	{
		writestring( m_fdServer, pszExtraHeaders );
	} /* if */
	writestring( m_fdServer, "\r\n" );
	writestring( m_fdServer, pszPostData );
	DebugTrace( 3, "done\n" );


    // Echo sent data to the debug console
    //
	DebugTraceStr( 6, "\r\n-------------- XML Sent To Server ----------------\r\n");
	DebugTraceStr( 6, pszPostData );
	DebugTraceStr( 6, "\r\n-------------- XML Sent To Server ----------------\r\n");


	// Allow a short time for the packet to be sent
	//
	OSTimeDly( TICKS_PER_SECOND/2 );


	// Get HTTP response.  We call the actual read with 1 second timeouts
    // in a loop so that we don't have long waits.
	//
	#define RX_BUFSIZE	1024
	char	RxBuffer[RX_BUFSIZE];
	int     BytesRead = 0;
	pszPostResponse[0] = '\0';

	m_bAbortFlag = FALSE;
	int iTimeout = iHttpTimeout * TICKS_PER_SECOND;
	while ( (m_fdServer > 0) &&
            (BytesRead < *pcbPostResponse - 1) &&
            (iTimeout > 0) )
	{
		// Check for abort signal, but only if we have not received anything yet.
		//
		if ( (BytesRead == 0) && m_bAbortFlag )
		{
			// Close the socket to terminate the connection
			//
			DebugTrace( 3, "\r\n%s: Aborting the current connection", timestamp() );
			close( m_fdServer );
			m_fdServer = 0;
			iRet = -10;
			break;
		} /* if */

		//DebugTrace( 3, "\r\nCalling SockReadWithTimeout, iTimer=%d", iTimer );
		int n = SockReadWithTimeout( m_fdServer,
                                     RxBuffer,
                                     RX_BUFSIZE,
                                     TICKS_PER_SECOND );
	    //DebugTrace( 3, "\r\nSockReadWithTimeout returned %d", n );
	    if (n > 0)
	    {
            // Received a chunk of data
            //
	    	BytesRead += n;
	        RxBuffer[n] = '\0';
	        DebugTrace( 3, "\r\n%s: BytesRead=%d", timestamp(), BytesRead );
	        if ((BytesRead + n) < *pcbPostResponse)
	        {
	            strncat( pszPostResponse, RxBuffer, n );
	        } /* if */
	        iTimeout -= TICKS_PER_SECOND;
	    } /* if */
	    else if ( n == 0 )
	    {
	    	// Timeout
            //
	    	if ( BytesRead > 0 )
	    		break;
	    	else
	    		iTimeout -= TICKS_PER_SECOND;
	    } /* else if */
	    else
	    {
	    	// Connection closed by server
            //
	    	DebugTrace( 2, "\r\n%s: Connection closed by server, n = %d", timestamp(), n);
	    	if ( iKeepAlive == 0 )
	    	{
	    		close( m_fdServer );
	    		m_ServerIp = 0; 	// Force DNS resolution next time
	    		m_fdServer = 0;
	    	} /* if */
            iRet = n;
            break;
	    } /* else */
	} /* while */
	pszPostResponse[BytesRead] = '\0';


    // Did we actually get any response data?
    //
	if ( BytesRead > 0 )
	{
        // Set return length
        //
        DebugTrace( 2, "\r\n%s: Received %d byte HTTP POST response", timestamp(), BytesRead );
		*pcbPostResponse = BytesRead;

        // Echo received data to the debug console
        //
		DebugTrace   ( 5, "\r\n------- RECEIVED (%3d bytes) ---------\r\n", BytesRead );
		DebugTraceStr( 5, pszPostResponse );
		DebugTrace   ( 5, "\r\n------- RECEIVED (%3d bytes) ---------\r\n", BytesRead );
        iRet = 0;
	} /* if */
	else
	{
		DebugTrace( 2, "\r\n%s: Error getting HTTP POST response, iRet=%d", timestamp(), iRet );
		m_ServerIp = 0; 	// Force DNS resolution next time
	} /* else */

	return iRet;

} /* ServerInterface::HttpPost() */


