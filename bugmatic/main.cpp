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


string	cached_download( string url, string fname )
{
	string		replyData;
	ifstream	downloadedjson;
	downloadedjson.open(fname);
	if( downloadedjson.is_open() )
	{
		cout << "\tUsing cached data from '" << fname << "'." << endl;
		replyData = file_contents( downloadedjson );
	}
	else
	{
		string		nextPage, lastPage;
		
		while( url != lastPage )
		{
			url_request	request;
			url_reply	reply;
			
			request.add_header( "User-Agent: bugmatic/0.1" );
		
			cout << "\tFetching URL: " << url << endl;
			CURLcode	errcode = request.load( url, reply );
			if( errcode == CURLE_OK )
			{
				replyData.append( reply.data() );
				
//				nextPage = reply.link_header_rel( "next" );
//				lastPage = reply.link_header_rel( "last" );
				if( lastPage.empty() )
					break;
				
				url = nextPage;
			}
			else
			{
				cerr << "Curl Error " << errcode << " downloading from '" << url << "'" << endl;
				return std::string();
			}
		}
	}
	
	ofstream	jsonfile(fname);
	jsonfile << replyData;

	return replyData;
}


void	download_comments( int bugNumber, string commentsURL )
{
	ifstream		downloadedcommentsjson;
	stringstream	downloadedcommentsfilename;
	downloadedcommentsfilename << "cache/" << bugNumber << "_comments_downloaded.json";
	string			commentsdata = cached_download( commentsURL, downloadedcommentsfilename.str() );
	
	stringstream	issuecommentfoldername;
	issuecommentfoldername << "issues/" << bugNumber << "_comments";
	mkdir( issuecommentfoldername.str().c_str(), 0777);
	
	string	errMsg;
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
	}
}


int main( int argc, const char * argv[] )
{
	try
	{
		mkdir("issues",0777);
		mkdir("milestones",0777);
		mkdir("users",0777);
		mkdir("cache",0777);
		
		string	replyData = cached_download( "https://api.github.com/repos/uliwitness/Stacksmith/issues?state=all&sort=created&direction=asc", "cache/issues.json" );
		
		string	errMsg;
		Json	replyJson = Json::parse( replyData, errMsg );
		if( errMsg.length() == 0 )
		{
			for( const Json& currItem : replyJson.array_items() )
			{
				int	bugNumber = currItem["number"].int_value();
				cout << "#" << bugNumber << ": " << currItem["title"].string_value();
				for( const Json& currLabel : currItem["labels"].array_items() )
				{
					cout << " [" << currLabel["name"].string_value() << "]";
				}
				cout << endl;
				
				if( currItem["comments"].int_value() > 0 )
				{
					download_comments( bugNumber, currItem["comments_url"].string_value() );
				}
				stringstream	issuefilename;
				issuefilename << "issues/" << bugNumber << ".json";
				ofstream		issuefile( issuefilename.str() );
				issuefile << currItem.dump();
			}
		}
		else
		{
			cerr << errMsg << endl;
		}
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
