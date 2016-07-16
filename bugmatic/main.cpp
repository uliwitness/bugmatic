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
#include <sstream>
#include <sys/stat.h>


using Json = json11::Json;
using namespace std;


string	file_contents( ifstream& stream )
{
	string	replyData;
	while( true )
	{
		int	currCh = stream.get();
		if( currCh == EOF )
			break;
		replyData.append(1, currCh);
		if( stream.eof() )
			break;
	}
	return replyData;
}


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
			replyData = file_contents( downloadedjson );
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
		
		mkdir("issues",0777);
		mkdir("milestones",0777);
		mkdir("users",0777);
		
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
				
				if( currItem["comments"].int_value() > 0 )
				{
					stringstream	issuecommentfoldername;
					issuecommentfoldername << "issues/" << bugNumber << "_comments";
					mkdir( issuecommentfoldername.str().c_str(), 0777);
					
					string			commentsdata;
					ifstream		downloadedcommentsjson;
					stringstream	downloadedcommentsfilename;
					downloadedcommentsfilename << bugNumber << "_comments_downloaded.json";
					downloadedcommentsjson.open(downloadedcommentsfilename.str());
					if( downloadedcommentsjson.is_open() )
					{
						cout << "Using cached data from "<< downloadedcommentsfilename.str() << endl;
						commentsdata = file_contents( downloadedcommentsjson );
					}
					else
					{
						url_request	request;
						url_reply	reply;
						
						request.add_header( "User-Agent: bugmatic/0.1" );
					
						CURLcode	errcode = request.load( currItem["comments_url"].string_value(), reply );
						if( errcode == CURLE_OK )
						{

							logfile << "status: " << reply.status() << endl;
							logfile << "type: " << reply.content_type() << endl;
							vector<string> headers = reply.headers();

							for(vector<string>::iterator it = headers.begin();
									it != headers.end(); it++)
								logfile << "Header: " << (*it) << endl;

							ofstream	jsonfile( downloadedcommentsfilename.str() );
							commentsdata = reply.data();
							jsonfile << commentsdata;
						}
						else
							cerr << "Curl Error: " << errcode << endl;
					}
					
					errMsg.erase();
					Json	commentsJson = Json::parse( commentsdata, errMsg );
					if( errMsg.length() == 0 )
					{
						for( const Json& currComment : commentsJson.array_items() )
						{
							stringstream	issuecommentfilename;
							issuecommentfilename << issuecommentfoldername.str() << "/" << currComment["id"].int_value() << ".json";
							string			fn = issuecommentfilename.str();
							ofstream	commentfile(fn);
							string		fc = currComment.dump();
							commentfile << fc;
						}
					}
					else
					{
						cerr << "Couldn't write comments for bug #" << bugNumber << endl;
						logfile << "Couldn't write comments for bug #" << bugNumber << endl;
					}
				}
				stringstream	issuefilename;
				issuefilename << "issues/" << bugNumber << ".json";
				ofstream		issuefile( issuefilename.str() );
				issuefile << currItem.dump();
				
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
