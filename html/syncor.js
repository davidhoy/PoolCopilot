//==============================================================================
//	Copyright (C) 2006 Syncor Systems, Inc.
//                    25500 Hawthorne Blvd. Ste 1158
//                    Torrance, CA 90505
//							 http://www.syncorsystems.com
// General Purpose Javascript Support Library Includes support
// for AJAX and JSON communication exchange. Relies on the prototype library.
//
// Part of the Syn"core" JavaScript library
//
//   $Author:: Tod Gentille           $
//     $Date:: 1/31/08 3:57p          $
// $Workfile:: syncor.js              $
// $Revision:: 18                     $ 
//
//

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.
//==============================================================================
if(typeof Prototype === undefined) {throw("syn requires the Prototype library  from prototype.js")};


//Create a pseudo namespace to hold all the helper methods
var syn = new Object();
//Constants for the syn library
syn.MIN_SEC_DELIM     = ":";
syn.BASE_TEN = 10;
syn.BASE_OCTAL= 8;
syn.BASE_HEX = 16;

//in final deployment set false - use interactive js to turn it on if needed
syn.DebugMsgOn_ = true;



//==============================================================================
//Dynamically add a javascript file to the head section of the current document.
//sourceFile - the pathname to the js source file you want to load. Same form
//that would be used if loading directly in head of document.
//==============================================================================
syn.ImportJsSourceFile = function(sourceFile)
{
  var scriptElem = document.createElement('script');
  scriptElem.setAttribute('src',sourceFile);
  scriptElem.setAttribute('type','text/javascript');
  document.getElementsByTagName('head')[0].appendChild(scriptElem);
};


//==============================================================================
// Wrapper for use by page elements that change pages. This will make it easier
// in the future if error checking for form changes and confirmation needs to 
// be added
//==============================================================================
syn.MoveToPage = function(url)
{
	//Example of how to check for changes - Currently not in use
//	var message = "Unsaved changes will be lost! Leave anyway?";
//	if (env.formDataChanged_)
//	{
//		if (!confirm(message)) return;
//	}
	location=url;
};

//==============================================================================
// Just a single place to store simple method to strip leading whitespace
//==============================================================================
syn.TrimLeading = function(stringToTrim)
{
	var trimmed  = stringToTrim.replace(/^\s+/,""); 
	return trimmed;
};

//==============================================================================
// Trim any leading white space from the passed value and if it has a length
// greater than 0 return true else return false. 
//==============================================================================
syn.ValidateNotBlank = function (userEntry)
{
	var trimmed_entry = syn.TrimLeading(userEntry);
	if (trimmed_entry.length > 0)return true;
	else return false
};

//==============================================================================
// General purpose way to add events to objects without overwriting any
// existing events that might be on that object.
//==============================================================================
syn.AddEvent = function(obj, eventType, functionToRun)
{ 
	if (obj.addEventListener)
	{
		obj.addEventListener(eventType, functionToRun, true);
		return true;
	} 
	else if (obj.attachEvent)
	{
		var ref = obj.attachEvent("on"+eventType, functionToRun);
		return ref;
	} 
	else 
	{
		return false;
	}
};


//------------------------------------------------------------------------------
// DHTML SECTION - A collection of simple methods for manipulating the DOM by 
// adding removing, cloning etc. Many routines here take either the id tag or 
// take an actual node.
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//==============================================================================
// For the passed node, remove any children that belong to that node. 
//==============================================================================
syn.RemoveAllChildren = function(node )
{
	if (node){ //make sure the node itself exists
		while(node.firstChild){ //see if the node has a child
			node.removeChild(node.firstChild); 
		}
	}
};


//==============================================================================
// Get a reference to the element node with the passed id and remove any 
// child nodes.
// @param idName - the id attribute on a tag. ie.  <div id ="">
//==============================================================================
syn.GetCleanNode = function(idName)
{
	try
	{
		var the_node = $(idName);
		syn.RemoveAllChildren(the_node);
		return  the_node;
	}
	catch (exception)
	{
		syn.Debug(arguments.callee.name+": "+exception.message);
	}
};

//==============================================================================
// Find the next sibling in the document that has the same tagname as the passed
// node
//==============================================================================
syn.GetNextSiblingTag = function (theNode)
{
	try
	{
		var next_node = theNode.nextSibling;
		while (next_node.tagName != theNode.tagName)
		{
			next_node = next_node.nextSibling;	
		}
		return next_node;		
	}
	catch (exception)
	{
		syn.Debug("syn.GetNextSiblingTag: "+exception.message);
	}

};

//==============================================================================
// Create text node from passed text and append that node to parent.
// @param parentNode - the node we will add to
// @param theText - the text we will add to node
//==============================================================================
syn.AddTextToNode = function(theText, parentNode)
{
		var text_node = document.createTextNode(theText);
		parentNode.appendChild(text_node);	
		return text_node;	
};

//==============================================================================
// Given an id string get the node and  attach
// the passed string to that node.
//==============================================================================
syn.AddTextToNodeViaId = function (theText, parentNodeId)
{
	var parent_node = $(parentNodeId);
	var text_node = syn.AddTextToNode(theText, parent_node);
	return text_node;
}; 


//==============================================================================
// Given an id string get the node and remove all its children adn then attach
// the passed string to that node.
//==============================================================================
syn.AddTextToCleanNodeViaId = function (theText, parentNodeId)
{
	var parent_node = syn.GetCleanNode(parentNodeId);
	var text_node = syn.AddTextToNode(theText, parent_node);
	return text_node;
};

//==============================================================================
// Remove children from node and then call AddTextToNode
//==============================================================================
syn.AddTextToCleanNode = function(theText, parentNode)
{
	syn.RemoveAllChildren(parentNode);
   var text_node =	syn.AddTextToNode(theText,parentNode);
   return text_node;
};


//==============================================================================
// Take the passed object, clone it and attache it to the passed ID
//==============================================================================
syn.AddCloneToCleanNode = function(idName,  theObject)
{
	var the_node = syn.GetCleanNode(idName);
	var clone_sel = theObject.cloneNode(true); //copy deep (for a select this would get select and options
	the_node.appendChild(clone_sel);
	return clone_sel;
};


//=================================================================================
// Given the ID of a Parent node that contains a  child <select> return the  
// selected index and the value field of that selected index in an object.
// @parentId - Id of the object that contains a <select> as the first child object
// @returns an object with two fields: index, value.
//=================================================================================
syn.SelectParentGetIndexAndValue = function (parentId)
{
   //Get the value from the selected item in the drop down list
   var parent_node = $(parentId);
   var sel_node = parent_node.childNodes[0];
   //create the return object with object literal syntax
   //and predefine the fields to show usage intent
   var select_data ={index:0, value:""}; 
   select_data.index = sel_node.selectedIndex;
   select_data.value = sel_node.options[select_data.index].value;
   return select_data;
};


//==============================================================================
// Given the id of a node get the node reference and  show it as "block"
//==============================================================================
syn.ShowMeBlock = function(nodeId)
{
	var the_node = $(nodeId);
	the_node.style.display="block";
};


//==============================================================================
// Given the id of a node get the node reference and  show it as "inline"
//==============================================================================
syn.ShowMeInline = function(nodeId)
{
	var the_node = $(nodeId);
	the_node.style.display="inline";
};



//==============================================================================
// Given the id of a node get the node reference and hide it.
//==============================================================================
syn.HideMe = function(nodeId)
{
	var the_node = $(nodeId);
	the_node.style.display="none";
};



//==============================================================================
// Scrolling to End of a text area. There were other techniques on the web
// that were browser dependent but I found that using the  scrollTop property
// works for both IE and Firefox. I am just using the character length of the
// text to set it because that should always be big enough. The param is really s
// supposed to be how far down you want to scroll (in pixels I assume)  
//==============================================================================
syn.ScrollToEnd = function(node) 
{
	if (node.scrollTop !== null)
	{
		var length = node.value.length *10;
		node.scrollTop = length;
	}
};


//==============================================================================
// Create a Select element that is composed of sequential numbers
// @param numDesired - the number of options the select area should have
//==============================================================================
syn.CreateSelectSequential = function(numDesired, startValue)
{
	var the_select = document.createElement("select");
	for(var i=0 ; i < numDesired; ++i)
	{
		var the_option = new Option(i+startValue, i+startValue); //display , value both set to i
		the_select.options[i]=the_option;
	}	
	return the_select;
};



//==============================================================================
// Creates an XHTML <select> element  from the passed array of properties, 
// @param optionList an array of value and display properties 
// @returns a fully valid select element from the DOM
//==============================================================================
syn.CreateSelect = function(optionList)
{
	try
	{
		var the_select = document.createElement("select");
		for (var i = 0 ; i< optionList.length; ++i)
		{      
			var the_option = new Option(optionList[i].display, optionList[i].value);
			//If you wanted an option to show up in the list but have it disabled you could do
			//something like this to disable the third item.
			//if (i == 3) the_option.disabled = true;
			the_select.options[i]=the_option;
		}
		return the_select;
	}
	catch (exception)
	{
		syn.Debug("syn.CreateSelect: "+exception.message);
	}
};

//==============================================================================   
// Given a selectNode return the "display" property that is currently selected. 
// returning the "value" property is trivial as it resides in the .value prop
// of the select. The display text can only be gotten through the options
// property 
//==============================================================================   
syn.SelectDisplayText = function (selectNode)
{
   return selectNode.options[selectNode.selectedIndex].text;
};



//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// END DHTML SECTION 
//------------------------------------------------------------------------------


//==============================================================================
// Use the JavaScript eval() function and the json string should turn into
// a valid object. Wrap it in a try clause in case it fails, we don't want
// any ugly errors we just return null which the caller must check for.
// @param jsonText the string containing the descriptor of the json object.
// a simple example would be '{"objname":{"name1":10, "name2":"value"}}
//==============================================================================
syn.EvalJsonObject = function(jsonText)
{
	var json_object = null;

	try
	{
		json_object = eval( "(" + jsonText + ")" );
	}
	catch (exception)
	{
		//just return the null object on error
		//TODO remember to set the var syn.DebugMsgOn_ to false when
		//you are ready to release your code.
		if (jsonText.length > 0)
		{
			syn.Debug("syn.EvalJsonObject: "+exception.message);
			syn.Debug("JSON object string: " + jsonText);
		}
	}
	return json_object;	
};


//==============================================================================
// Ajax code using the prototype library. This is the only method needed
// for doing Ajax calls. The Ajax object and specifically the Ajax.Request
// method allows us to specify both the request and the function to be called
// on completion. We wrote this wrapper so that the completion function could
// be passed in and therefore used by all requests in the entire system. 
//==============================================================================
syn.MakeAjaxRequest = function(theParams, completionFunction, callingScope)
{
	try
	{
		//If the completionFunction needs to know the calling scope (i.e. what was "this"
		// when the request was issued, then the request passed the calling scope into
		// this method. 
		var pass_scope = false;
		if (arguments[2])
		{
			pass_scope = true;
		}

		// IE does overly aggressive caching and if it sees the same AJAX request it will
		//just serve us data out of the cache even if we use the no-cache meta tag
		//the preferred way to solve this is with the requestHeaders setting shown below
		new Ajax.Request("ajax.htm",
								{	method:"get",
									parameters:theParams,
									requestHeaders:["If-Modified-Since","Sat, 1 Jan 2000 00:00:00 GMT"], 
									//requestTimeout: 5,
									//onTimeout: syn.AjaxTimeoutLoadErrorPage,
									onComplete: function (transport)
									{
										//if pass_scope is true when use the call() method (a Javascript feature) so we can 
										//pass back the scope we want completionFunction to use
										if (pass_scope) 
										{
											completionFunction.call(callingScope,transport.responseText);
										}
										else completionFunction(transport.responseText);
									}
									
								}); //end of new Ajax.Request
	}
	catch (exception)
	{
		syn.Debug("syn.MakeAjaxRequest: "+exception.message+" ");
	}           			               
};

//==============================================================================
//==============================================================================
syn.AjaxTimeout = function()
{
 	alert ("Communication Failure - Check that the instrument is plugged in and functioning.\nYou may need to reload the page to recover.");
};




//==============================================================================
// A very simplistic way to show the name/value pairs in an object. If the value
// is an object it recurses. If the object is an Array it doesn't, because Arrays are 
// rather verbose objects and we just want to see a list of the array values and 
// the standard value will give us that. Arrays that hold arrays of Objects will
// just return the [Object object] string so any calling methods that have 
// arrays of objects will have to handle them themselves. This method is intended
// for early debugging of JSON strings that are passed back from the instrument.
// This allows a quick way to see whats in the JSON object that was created. 
// @param theObj - the JavaScript object we want to examine - typically created
// be eval()ing a JSON string but it doesn't have to be. 
// @returns the full description string
//==============================================================================
syn.DescribeObject = function(theObj)
{
	var description ="";
	for (var property_name in theObj)  //just step through every property in the object
	{
		var property_value = theObj[property_name]; //use the indexer to get the property value

		if (property_value instanceof Array) //std op for an Array
		{
			description += property_name + "="+property_value+"<br />";
		}
		//if it's an object recurse (otherwise it would just return [Object])
		else if (property_value instanceof Object) 
		{
			description += property_name + ": "+property_value+"<br />";
			if (property_name.indexOf("extend")< 0)
			{ 
		      description += syn.DescribeObject(property_value);
		    }
		}
		else //in our JSON spec there are no other types, other than simple, so just get the value
		{
			description += property_name + "="+property_value+"<br />";
		}
	}		
	return description;
};


//==============================================================================
// For demo purposes only. See StartClock () for how this is invoked by passing
// it to syn.MakeAjaxRequest
//==============================================================================
syn.UpdateAjaxTestArea = function(theData)
{
	$('ajaxTest').innerHTML= theData;
};




//==============================================================================
// A general purpose method for displaying debug information. If there isn't
// a member property of this function called "box" we will create one and 
// set up a div tag with id, class and styling. Then on all subsequent calls
// the box property will persist (similar to a static local var in C++). Then
// we just create the text node with the message and append it to the div
// If we don't have a body tag for all this to attach to we just show an alert()
//==============================================================================
syn.Debug = function(message)
{
   if (syn.DebugMsgOn_)
   {
	   if (document.body)
	   {
		   if (!this.box) //store box property as part of function 	
		   {// it will be here after the first call so we can just append to it
			   this.box = document.createElement("div");
			   this.box.setAttribute("class","debugbox");
			   this.box.setAttribute("id","thedebugbox");
			   //this.box.style.marginTop="150px";
			   this.box.style.position="relative";
            this.box.style.top ="50px"; //todo teg move it out of the way
			   document.body.appendChild(this.box);
			   this.box.innerHTML = "<h1 style ='text-align:center'> Debug Message Area </h1>";
		   }
		   var new_node = document.createElement("div");
		   syn.AddTextToNode(message, new_node);
		   this.box.appendChild(new_node);
	   }
	   else //no body tag available
	   {
		   alert (message);
	   } 
	}
};


//==============================================================================
// This method will take any number of arguments and turn them into a URL
// parameter string of the form "?name=value&name2=value2. It is expected that 
// if the user wants back a fully valid string they will pass in an even
// number of arguments.
// @paramter any number of string arguments
// @returns the url parameter list string
//==============================================================================
syn.MakeUrlParmList = function(variableNumArgs)
{
	var url_param_list="";
	var sep ="=";
	for (i=0, maxArgs=arguments.length; i < maxArgs; ++i)
	{
		url_param_list += arguments[i] ;
		if (i<(maxArgs-1)) url_param_list += sep; //add separator on all but last param
		sep = (sep =="=") ? "&":"=";  //toggle url argument separator between = and &
	}
	return url_param_list;
};




//==============================================================================
// The DiscardResponse is called for ajax requests that don't really care about
// anything that comes back. Can be useful for debugging to know when a request
// completes 
//==============================================================================
syn.DiscardAjaxResponse = function(responseText)
{
	var the_response = responseText; //something for the debugger to break on.
	syn.Debug ("syn.DiscardResponse: "+ responseText);
};



//==============================================================================
// Using an image object we can set the onLoad event to call another routine
// that actually loads the image. This way if the src doesn't lead to a valid
// image nothing happens (avoids the placeholder image for invalid src)
// @param theNode to which we will attach the image
// @param fullImgPath the src path to check
//==============================================================================
syn.ImageLoadIfValid = function(theNode, fullImgPath)
{
   //if we get passed an empty string - allow it to clear the background image.
   if ( fullImgPath.length === 0) syn.ImageLoadToStyleBackground(theNode,"");
   else
   {
	   //therefore use the onLoad event of a new image to load the image. If image doesn't exist
	   //onError will get invoked which we haven't defined because we do nothing if the image doesn't exist.
	   var test_image = new Image();
	   test_image.node = theNode;
	   test_image.path = fullImgPath;
	   //test_image.onLoad = function(){syn.ImageLoadToStyleBackground(theNode, fullImgPath);};//syn.ImageLoadToStyleBackground(theNode, fullImgPath);
	   test_image.onload = syn.ImageLoadToStyleBackground2; //when invoked this will equal test_image
	   test_image.onerror = syn.ImageLoadError;
	   test_image.src = fullImgPath;
   }
};

//==============================================================================
// If you need to load an image you know exists directly to the background
// use this version. 
//==============================================================================
syn.ImageLoadToStyleBackground = function(theNode, fullImgPath)
{
   theNode.style.backgroundImage = 'url('+fullImgPath+')';	
};

//==============================================================================
// The onLoad handler for a test image that gets called when the src path
// is valid. This one loads to a style backgroundImage
//==============================================================================
syn.ImageLoadToStyleBackground2 = function()
{
   this.node.style.backgroundImage = 'url('+this.path+')';	
};

//==============================================================================
// When syn.ImageLoadIfValid is used to load an image this is the error handler
// it just displays an error to our debug DIV. 
//==============================================================================
syn.ImageLoadError = function ()
{
	syn.Debug("load image error: "+this.path);
};

//==============================================================================
// Given a standard JavaScript date variable turn it into a string of the form
// mm/dd/yyyy hh:mm:ss 
//==============================================================================
syn.CreateNumericDateTimeString = function(theJsDate)
{
   var the_day  = theJsDate.getDate();
   the_day = syn.ForceToTwoDigits(the_day);
   var the_month = theJsDate.getMonth() + 1; //returns 0 for January 
   the_month = syn.ForceToTwoDigits(the_month);
   var the_year = theJsDate.getFullYear();
   var the_hour = theJsDate.getHours();
   the_hour = syn.ForceToTwoDigits(the_hour);
   var the_minutes = theJsDate.getMinutes();
   the_minutes = syn.ForceToTwoDigits(the_minutes); 
   var the_seconds = theJsDate.getSeconds();
   the_seconds = syn.ForceToTwoDigits(the_seconds); 
   var date_time_str = the_month+ "/"+the_day + "/" + the_year + " ";
   date_time_str    += the_hour +":" + the_minutes + ":" + the_seconds;
   return date_time_str;   
};

//==============================================================================
// Take the passed number string and if it is < 10 prepend a leading 0 and return
// the string. 
// NOTE calling method should ensure that passed string contains only digits.
//==============================================================================
syn.ForceToTwoDigits = function(numToCheck)
{
   if (numToCheck <10) numToCheck = "0" + numToCheck;
   return numToCheck;
};


//==============================================================================
// Browser independent method of getting the keypress event. Mozilla browsers
// pass it as an event in the onkeypress handler, IE keeps it in the window.event
// each type holds the actual value of the keypress differently as shown in the code
//==============================================================================
syn.GetKeypressKey = function(keyEvent)
{
   if (keyEvent.which) return keyEvent.which;
   if (window.event) return window.event.keyCode;
   else return null;  
};


//==============================================================================
// Filter all keypresses in the passed range and also any that match any of the 
// optional parameters after the endRange
// @param keyEvent - Mozilla based browsers pass a key event (IE Doesn't)
// @param startRange - the first valid ASCII value
// @param endRange - the last valid ASCII value
// @param any others - all other parameters are treated as additional values
// that should be filtered. So if you want everything in 0x20 to 0x40 but don't 
// want to allow * (0x2A), you would pass 0x2A as the fourth parameter. There
// can be any number of these optional parameters
//==============================================================================
syn.FilterKeyRange = function (keyEvent, startRange, endRange)
{

   var keypress = syn.GetKeypressKey(keyEvent);
   if (keypress === null) return true; //certain keys (like tab/delete get handled prior to our handler)
   
   if (keypress =='\t') return true ; //allow next field to be navigated to
   if (keypress =='0x8') return true ; //allow backspace key  
   if (keypress == 13) return true; //allow enter -attempt to allow form submit

   if (keypress <startRange) return false;
   if (keypress > endRange) return false;
   max_args = arguments.length;
   //check optional parameters and return false if any of them are found
   for (var i = 3; i < max_args; ++i)
   {
      if (keypress == arguments[i]) return false;
   }
   return true;
   
};


//==============================================================================
// Use the IE only clientInformation property (called navigator in other apps)
// and get the appVersion. If it contains MSIE 5.0 or MSIE6.0 return true
//==============================================================================
syn.IsIeV6OrV5 = function()
{
	//only IE supports the clientInformation property
	if (typeof(clientInformation) == 'undefined') return false;

	var client_info = clientInformation.appVersion; //typically only supported by MS

	if (client_info.indexOf("MSIE 6.0") != -1) return true;
	if (client_info.indexOf("MSIE 5.0") != -1) return true;
	return false;

};



