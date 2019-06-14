/*
 * HtmlServices.h
 *
 *  Created on: Nov 18, 2009
 *      Author: Tod
 */

#ifndef HTMLSERVICES_H_
#define HTMLSERVICES_H_

#include <vector>
#include <map>
#include <iosfwd>


class HtmlServices
{
	public:
		HtmlServices();
		//static void GetSampleHtmlTable(std::ostringstream& response);
		static int PostHandler(int sock, char *url, char *pData, char *rxBuffer);
		void RegisterPost() const;
		//static void WriteJsonMachineState(int sock);
		static void WriteJsonPoolData(int sock);
		static void WriteJsonHistoryData(int sock);
		static void WriteJsonAuxData(int sock);
		static void WriteJsonPhControlData(int sock);
		static void WriteJsonAutochlorData(int sock);
		static void WriteJsonIonisationData(int sock);
		static void WriteJsonRefillData(int sock);
		static void WriteJsonOrpControlData(int sock);

		//static void WriteDebugOptionsJsonData(int sock);
		static void ProcessAjaxFunction(int sock, std::string url );

	private:
		static bool _testCheckbox;
		typedef void (*ptrToAjaxFunction)(int sock);
		static const std::string _requestToken;

		//static double GetRandomNumber();

		typedef std::map<std::string,ptrToAjaxFunction> AjaxFunctionMap ;
		static AjaxFunctionMap AjaxCommands;

};

#endif /* HTMLSERVICES_H_ */
