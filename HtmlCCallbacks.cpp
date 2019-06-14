/*
 * HtmlCCallbacks.c
 *
 *  Created on: Dec 9, 2009
 *      Author: Tod
 */

#include <iosys.h> //for writestring
#include <sstream>
#include <iostream>

#include "HtmlServices.h"
#include "PoolCopInterface.h"


using namespace std;
//==============================================================================
// Simple sample code to show how to implement a function callback from
// webpage
//==============================================================================
extern "C"
{
	void WriteStartupBanner(int sock, const char* url)
	{
		ostringstream display_stream;
		display_stream << "PoolCopilot v1.0";
		writestring(sock, display_stream.str().c_str());
	}

	//==============================================================================
	// A callback method from the web page.
	// Send hidden info from the server to the browser. Used to allow javascripts
	// to setup gui elements to values that reflect the state of the machine.
	// Data is stored in an object with properties using object literal syntax like
	// var machineState_ =
	// {
	//     debugState: 3,
	//     checkboxState: true
	// };
	// @param sock passed in by NB 0 the socket we will write out to
	// @param url - unused.
	//==============================================================================
	void WriteSystemVarsToJavaScript(int sock, const char* url)
	{
		//cout << endl << "WriteSystemVarsToJavaScript() has been called" << endl;
		//HtmlServices::WriteJsonMachineState(sock);
	}



	/*
	void JavaScriptPumpOn( int sock, const char* url)
	{
		//cout << endl << "JavaScriptPumpOn() has been called" << endl;
		g_PoolCop.ControlPump( "on" );
	}
	*/


	/*
	void JavaScriptPumpOff( int sock, const char* url)
	{
		//cout << endl << "JavaScriptPumpOff() has been called" << endl;
		g_PoolCop.ControlPump( "off" );
	}
	*/


	//==============================================================================
	// Example function that can be called via AJAX. This one just writes out the
	// the current timetick value but of course this could be much more elaborate.
	//==============================================================================
	void AjaxCallback(int sock, const char* url)
	{
		HtmlServices::ProcessAjaxFunction(sock, url);
		return;
	}

}
