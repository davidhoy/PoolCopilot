/* Rev:$Revision: 1.1 $ */
/******************************************************************************
 * Copyright 2010 NetBurner, Inc  ALL RIGHTS RESERVED
 *   Permission is hereby granted to purchasers of NetBurner Hardware
 *   to use or modify this computer program for any use as long as the
 *   resultant program is only executed on NetBurner provided hardware.
 *
 *   No other rights to use this program or it's derivatives in part or
 *   in whole are granted.
 *
 *   It may be possible to license this or other NetBurner software for
 *   use on non NetBurner Hardware. Please contact sales@netburner.com
 *   for more information.
 *
 *   NetBurner makes no representation or warranties with respect to
 *   the performance of this computer program, and specifically disclaims
 *   any responsibility for any damages, special or consequential,
 *   connected with the use of this program.
 *
 *   NetBurner, Inc.
 *   5405 Morehouse Dr
 *   San Diego CA, 92121
 *   USA
 *****************************************************************************/
#include "predef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basictypes.h>
#include <htmlfiles.h>
#include <http.h>
#include <iosys.h>


// Buffers to hold data posted from form
#define MAX_BUF_LEN 80
char textForm1[MAX_BUF_LEN];
char textForm2[MAX_BUF_LEN];

// Functions called from a web page must be declared extern C
extern "C"
{
   void WebTextForm1( int sock, PCSTR url );
   void WebTextForm2( int sock, PCSTR url );
};


// Function to display value in web page
void WebTextForm1( int sock, PCSTR url )
{
   writestring( sock, textForm1 );
}

// Function to display value in web page
void WebTextForm2( int sock, PCSTR url )
{
   writestring( sock, textForm2 );
}


/*-------------------------------------------------------------------
 * MyDoPost() callback function
 * The NetBurner web server will call this function whenever a
 * web browser posts form data.
 * - This is where you have the ability to parse the url and
 *   form data sent in the HTML Post.
 * - The TCP connection will be held open while you are in this function
 *   so you can take whatever action you wish, or send any data you
 *   want to the web browser.
 * - IMPORTANT: you need to provide some type of content (eg web page)
 *   to the web browser or send a redirect command to the web browser
 *   so that it will issue an HTML GET request to obtain a web page.
 *   Otherwise the use will be left looking at a blank page.
 * - This example illustrates how to process multiple HTML Forms,
 *   so it parses the url to determine the name of the form, which
 *   is defined by the HTML Form action tag.
 *-----------------------------------------------------------------*/
int MyDoPost( int sock, char *url, char *pData, char *rxBuffer )
{
   // Display the data passed in just for the purpose of this example
   iprintf("----- Processing Post -----\r\n");
   iprintf("Post URL: %s\r\n", url);
   iprintf("Post Data: %s\r\n", pData);

   /* Parse the url to determine which form was used. The item
    * we are parsing for is defined by the HTML Form tag called
    * "action". It can be any name, but in this example we are
    * using it to tell us what web page the form was sent from.
    * Note that httpstricmp() requires the search string to
    * be in upper case, even if the action tag is lower case.
    */
   if ( httpstricmp( url + 1, "FORM1" ) )
   {
      ExtractPostData( "textForm1", pData, textForm1, MAX_BUF_LEN );
      iprintf("textForm1 set to: \"%s\"\r\n", textForm1 );

      // Tell the web browser to issue a GET request for the next page
      RedirectResponse( sock, "page2.htm" );
   }
   else if ( httpstricmp( url + 1, "FORM2" ) )
   {
      ExtractPostData( "textForm2", pData, textForm2, MAX_BUF_LEN );
      iprintf("textForm2 set to: \"%s\"\r\n", textForm2 );

      RedirectResponse( sock, "complete.htm" );
   }
   else
   {
	   NotFoundResponse( sock, url );
	   iprintf("We did not match any page\r\n");
   }

   return 1;
}


void RegisterPost()
{
   SetNewPostHandler( MyDoPost );
}








