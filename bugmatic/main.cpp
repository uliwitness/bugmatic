//
//  main.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 04/07/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include "url_request.hpp"
#include "json11.hpp"
#include <fstream>


using Json = json11::Json;
using namespace std;


int main( int argc, const char * argv[] )
{
	try
	{
		ofstream	logfile("bugmatic.log");
		
		string		replyData;
		ifstream	downloadedjson;
		downloadedjson.open("downloaded.json");
		if( downloadedjson.is_open() )
		{
			cout << "Using cached data from 'downloaded.json'." << endl;
			while( true )
			{
				int	currCh = downloadedjson.get();
				if( currCh == EOF )
					break;
				replyData.append(1, currCh);
				if( downloadedjson.eof() )
					break;
			}
		}
		else
		{
			url_request	request;
			url_reply	reply;
			
			request.add_header( "User-Agent: bugmatic/0.1" );
		
			CURLcode	errcode = request.load( "https://api.github.com/repos/uliwitness/Stacksmith/issues?state=all&sort=created&direction=asc", reply );
			if( errcode == CURLE_OK )
			{

				logfile << "status: " << reply.status() << endl;
				logfile << "type: " << reply.content_type() << endl;
				vector<string> headers = reply.headers();

				for(vector<string>::iterator it = headers.begin();
						it != headers.end(); it++)
					logfile << "Header: " << (*it) << endl;

				ofstream	jsonfile("downloaded.json");
				
				replyData = reply.data();
				
			}
			else
				cerr << "Curl Error: " << errcode << endl;
		}

		//logfile << "<<" << replyData << ">>" << endl;

		string	errMsg;
		Json	replyJson = Json::parse( replyData, errMsg );
		if( errMsg.length() == 0 )
		{
			for( const Json& currItem : replyJson.array_items() )
			{
				int	bugNumber = currItem["number"].int_value();
				logfile << "#" << bugNumber << ": " << currItem["title"].string_value();
				for( const Json& currLabel : currItem["labels"].array_items() )
				{
					logfile << " [" << currLabel["name"].string_value() << "]";
				}
				logfile << endl;

				logfile << "----------" << endl;
			}
		}
		else
		{
			logfile << errMsg << endl;
			cerr << errMsg << endl;
		}
		
		logfile << "Done." << endl;
		
		logfile.flush();
	}
	catch( const std::exception& err )
	{
		cerr << err.what() << endl;
	}
	catch( ... )
	{
		cerr << "Unknown exception" << endl;
	}
	
	cout << "Done." << endl;
	
	return 0;
}
