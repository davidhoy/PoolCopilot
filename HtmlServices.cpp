/*
 * HtmlServices.cpp
 *
 *  Created on: Nov 18, 2009
 *      Author: Tod
 */

#include "HtmlServices.h"

#include <iostream>
#include <sstream>
#include <utils.h> //has TimeTick
//#include <string>
#include <iosys.h> //for writestring
#include <constants.h>
#include <htmlfiles.h>
#include <http.h>
//#include <map>
//#include <vector>
#include "PoolCop.h"
#include "PoolCopDataBindings.h"

using namespace std;

HtmlServices::AjaxFunctionMap HtmlServices::AjaxCommands;
const string                  HtmlServices::_requestToken = "request";
bool                          HtmlServices::_testCheckbox = true;
const string 				  TERM_STRING = "\r\n";


HtmlServices::HtmlServices()
{
	AjaxCommands.insert(make_pair(_requestToken+"=poolData",       &WriteJsonPoolData));
	AjaxCommands.insert(make_pair(_requestToken+"=auxData",        &WriteJsonAuxData));
	AjaxCommands.insert(make_pair(_requestToken+"=phControl",      &WriteJsonPhControlData));
	AjaxCommands.insert(make_pair(_requestToken+"=autochlorData",  &WriteJsonAutochlorData));
	AjaxCommands.insert(make_pair(_requestToken+"=refillData",     &WriteJsonRefillData));
	AjaxCommands.insert(make_pair(_requestToken+"=orpControl",     &WriteJsonOrpControlData));
	AjaxCommands.insert(make_pair(_requestToken+"=historyData",    &WriteJsonHistoryData));
	AjaxCommands.insert(make_pair(_requestToken+"=ionisationData", &WriteJsonIonisationData));
}


void HtmlServices::ProcessAjaxFunction(int sock, string url)
{
	string::size_type found_string = url.find(_requestToken);
	if (found_string == string::npos)
	{
		ostringstream msg;
		msg << "Url did not contain " << _requestToken << endl;
		//Debug::Write(msg.str());
		return;
	}

	string command = url.substr(found_string);
	AjaxFunctionMap::iterator ajax_iter = AjaxCommands.find(command);
	if (ajax_iter != AjaxCommands.end())
	{
		//cout << "found a command " << ajax_iter->first << endl;
		ajax_iter->second(sock);
	}
	else
	{
		cout << "could not find match for " << command << endl;
	}
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonPoolData(int sock)
{
	ostringstream java_script;

	char Time[40], Date[40];
	sprintf( Time, "\"%02d:%02d:%02d\"",
		     g_PoolCopData.dynamicData.Time.Hour,
			 g_PoolCopData.dynamicData.Time.Minute,
			 g_PoolCopData.dynamicData.Time.Second );
	sprintf( Date, "\"%02d/%02d/%04d\"",
			 g_PoolCopData.dynamicData.Date.Day,
			 g_PoolCopData.dynamicData.Date.Month,
			 g_PoolCopData.dynamicData.Date.Year );

	java_script << "{\n";
	java_script << "\t " << "Time:"         << Time                                                 << ",\n";
	java_script << "\t " << "Date:"         << Date                                                 << ",\n";
	java_script << "\t " << "FilterPos:\""  << g_PoolCopData.dynamicData.ValvePosition              << "\",\n";
	java_script << "\t " << "PumpState:\""  <<(g_PoolCopData.dynamicData.PumpState ? "On" : "Off")  << "\",\n";
	java_script << "\t " << "Pressure:\""   << g_PoolCopData.dynamicData.PumpPressure               << " BAR\",\n";
	java_script << "\t " << "WaterTemp:\""  << g_PoolCopData.dynamicData.WaterTemperature           << " C\",\n";
	java_script << "\t " << "pH:"           << g_PoolCopData.dynamicData.pH                         << ",\n";
	java_script << "\t " << "ORP:"          << g_PoolCopData.dynamicData.Orp                        << ",\n";
	java_script << "\t " << "BattVolts:\""  << g_PoolCopData.dynamicData.BatteryVoltage             << " VDC\"\n";
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonHistoryData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	for ( int i = 0; i < AUX_COUNT; i++ )
	{
		java_script << "\t " << "Aux" << i+1 << "Name:\""  <<  g_PoolCopData.auxSettings.Aux[i].Label << "\",\n";
		java_script << "\t " << "Aux" << i+1 << "State:\"" << (g_PoolCopData.auxSettings.Aux[i].Status ? "On" : "Off") << "\"";
		if ( i < AUX_COUNT )
			java_script << ",\n";
		else
			java_script << "\n";
	}
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}

//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonAuxData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	for ( int i = 0; i < AUX_COUNT; i++ )
	{
		java_script << "\t " << "Aux" << i+1 << "Name:\""  <<  g_PoolCopData.auxSettings.Aux[i].Label << "\",\n";
		java_script << "\t " << "Aux" << i+1 << "State:\"" << (g_PoolCopData.auxSettings.Aux[i].Status ? "On" : "Off") << "\"";
		if ( i < AUX_COUNT )
			java_script << ",\n";
		else
			java_script << "\n";
	}
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonPhControlData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	java_script << "\t " << "Installed:\""     <<  g_PoolCopData.pHControl.Installed     << "\",\n";
	java_script << "\t " << "SetPoint:\""      <<  g_PoolCopData.pHControl.SetPoint      << "\",\n";
	java_script << "\t " << "pHType:\""        <<  g_PoolCopData.pHControl.pHType        << "\",\n";
	java_script << "\t " << "DosingTime:\""    <<  g_PoolCopData.pHControl.DosingTime    << "\",\n";
	java_script << "\t " << "LastInjection:\"" <<  g_PoolCopData.pHControl.LastInjection << "\"\n";
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonAutochlorData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	/*
	java_script << "\t " << "Installed:\""    <<  g_PoolCopData.pHControl.Installed    << "\",\n";
	java_script << "\t " << "State:\""        <<  g_PoolCopData.pHControl.State        << "\",\n";
	java_script << "\t " << "pH:\""           <<  g_PoolCopData.pHControl.pH           << "\",\n";
	java_script << "\t " << "SetPoint:\""     <<  g_PoolCopData.pHControl.SetPoint     << "\",\n";
	java_script << "\t " << "SetPointTemp:\"" <<  g_PoolCopData.pHControl.SetPointTemp << "\",\n";
	java_script << "\t " << "Mode:\""         <<  g_PoolCopData.pHControl.pHMode       << "\",\n";
	java_script << "\t " << "MaxDosing:\""    <<  g_PoolCopData.pHControl.MaxDosing    << "\",\n";
	java_script << "\t " << "CalcDosing:\""   <<  g_PoolCopData.pHControl.CalcDosing   << "\"\n";
	*/
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonRefillData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	/*
	java_script << "\t " << "Installed:\""    <<  g_PoolCopData.pHControl.Installed    << "\",\n";
	java_script << "\t " << "State:\""        <<  g_PoolCopData.pHControl.State        << "\",\n";
	java_script << "\t " << "pH:\""           <<  g_PoolCopData.pHControl.pH           << "\",\n";
	java_script << "\t " << "SetPoint:\""     <<  g_PoolCopData.pHControl.SetPoint     << "\",\n";
	java_script << "\t " << "SetPointTemp:\"" <<  g_PoolCopData.pHControl.SetPointTemp << "\",\n";
	java_script << "\t " << "Mode:\""         <<  g_PoolCopData.pHControl.pHMode       << "\",\n";
	java_script << "\t " << "MaxDosing:\""    <<  g_PoolCopData.pHControl.MaxDosing    << "\",\n";
	java_script << "\t " << "CalcDosing:\""   <<  g_PoolCopData.pHControl.CalcDosing   << "\"\n";
	*/
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}



//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonOrpControlData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	/*
	java_script << "\t " << "Installed:\""    <<  g_PoolCopData.pHControl.Installed    << "\",\n";
	java_script << "\t " << "State:\""        <<  g_PoolCopData.pHControl.State        << "\",\n";
	java_script << "\t " << "pH:\""           <<  g_PoolCopData.pHControl.pH           << "\",\n";
	java_script << "\t " << "SetPoint:\""     <<  g_PoolCopData.pHControl.SetPoint     << "\",\n";
	java_script << "\t " << "SetPointTemp:\"" <<  g_PoolCopData.pHControl.SetPointTemp << "\",\n";
	java_script << "\t " << "Mode:\""         <<  g_PoolCopData.pHControl.pHMode       << "\",\n";
	java_script << "\t " << "MaxDosing:\""    <<  g_PoolCopData.pHControl.MaxDosing    << "\",\n";
	java_script << "\t " << "CalcDosing:\""   <<  g_PoolCopData.pHControl.CalcDosing   << "\"\n";
	*/
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Example of how to write out a well formed JSON object that the receiving script
// can just use eval() on to create a variable. Then the script can easily extract
// the values and use them on the page.
// In SupportScripts.js you will see function CreateSampleData(resultData)
// that gets set as the responder to our AJAX call and it processes the data created
// here.
//==============================================================================
void HtmlServices::WriteJsonIonisationData(int sock)
{
	ostringstream java_script;

	java_script << "{\n";
	/*
	java_script << "\t " << "Installed:\""    <<  g_PoolCopData.pHControl.Installed    << "\",\n";
	java_script << "\t " << "State:\""        <<  g_PoolCopData.pHControl.State        << "\",\n";
	java_script << "\t " << "pH:\""           <<  g_PoolCopData.pHControl.pH           << "\",\n";
	java_script << "\t " << "SetPoint:\""     <<  g_PoolCopData.pHControl.SetPoint     << "\",\n";
	java_script << "\t " << "SetPointTemp:\"" <<  g_PoolCopData.pHControl.SetPointTemp << "\",\n";
	java_script << "\t " << "Mode:\""         <<  g_PoolCopData.pHControl.pHMode       << "\",\n";
	java_script << "\t " << "MaxDosing:\""    <<  g_PoolCopData.pHControl.MaxDosing    << "\",\n";
	java_script << "\t " << "CalcDosing:\""   <<  g_PoolCopData.pHControl.CalcDosing   << "\"\n";
	*/
	java_script << "}";

	writestring(sock, java_script.str().c_str());
}


//==============================================================================
// Post Handler - When the user submits a form this method is called
// Set up using SetNewPostHandler() via the RegisterPost() method
//==============================================================================
int HtmlServices::PostHandler(int sock, char *url, char *pData, char *rxBuffer)
{
	const string REDIRECT_PAGE = "index.htm";
	cout << "In MyDoPost..." << endl;
	//In a Real application one would likely have multiple forms.
	//one would then select the proper processing function by dispatching based
	//on the url data member.
	//In this case we will ignore that....
	const int MAX_CB_RET_LEN = 3; //Checkboxes only exist in returned post if the box is enabled.
	const int MAX_DEBUG_SIZE_LEN = 16; //65536 wold be our biggest value so make it one extra
	//Declare the strings for all the HTML interface widget names
	const string TEST_CB = "ckb_test";
	const string DEBUG_CMB = "cmb_debug";

	//ExtractPostData returns -1 if field not found, 0 if found but empty
	char checkbox_data[MAX_CB_RET_LEN];
	char debug_setting[MAX_DEBUG_SIZE_LEN];

	//Checkboxes only show up in the post if they are true, so just look to see if it is here
	if (ExtractPostData(TEST_CB.c_str(), pData, checkbox_data, sizeof(checkbox_data)) != -1)
	{
		_testCheckbox = true;
	}
	else _testCheckbox = false;

	if (ExtractPostData(DEBUG_CMB.c_str(), pData, debug_setting, sizeof(debug_setting)) != -1)
	{
		const string scpi_debug_prefix = "SYST:DEBUG  ";
		ostringstream scpi_cmd;
		scpi_cmd << scpi_debug_prefix << " " << debug_setting;
		cout << "SCPI command: " << scpi_cmd.str() << endl;
//		Debug::SetLevel(debug_setting);
	}
	else
	{
		cout << "Error in extracting text from debug setting popup." << endl;
	}

	//We have to respond to the post with a new HTML page...
	//In this case we will redirect so the browser will go to that URL for the response...
	RedirectResponse(sock, REDIRECT_PAGE.c_str());

	return 0;
}

//==============================================================================
// Install the method we want to have handle form submissions
//==============================================================================
void HtmlServices::RegisterPost() const
{
	SetNewPostHandler(PostHandler);
}

