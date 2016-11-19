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
#include <stdio.h>
#include "fake_filesystem.hpp"	// until <filesystem> becomes available.


#define USER_AGENT		"bugmatic/0.1"


using Json = json11::Json;
using namespace std;
using namespace fake;


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


void	paged_cached_download( string url, string fname, string userName, string password, std::function<void(string)> fileContentsCallback )
{
	ifstream	downloadedjson;
	downloadedjson.open(fname);
	if( downloadedjson.is_open() )
	{
		size_t	x = 1;
		while( true )
		{
			ifstream		currDownloadedjson;
			stringstream	currFilename;
			if( x == 1 )
				currFilename << fname;
			else
			{
				off_t			dotOffs = fname.rfind('.');
				currFilename << fname.substr(0,dotOffs) << x << fname.substr(dotOffs);
			}
			currDownloadedjson.open(currFilename.str());
			if( !currDownloadedjson.is_open() )
				break;
			cout << "\tUsing cached data from '" << currFilename.str() << "'." << endl;
			fileContentsCallback( file_contents( currDownloadedjson ) );
			
			x++;
		}
	}
	else
	{
		string		nextPage, lastPage;
		size_t		x = 1;
		
		while( true )
		{
			url_request	request;
			url_reply	reply;
			
			request.add_header( "User-Agent: " USER_AGENT );
			request.set_user_name( userName );
			request.set_password( password );
		
			cout << "\tFetching URL: " << url << endl;
			CURLcode	errcode = request.load( url, reply );
			if( errcode == CURLE_OK )
			{
				stringstream	currFilename;
				if( x == 1 )
					currFilename << fname;
				else
				{
					off_t			dotOffs = fname.rfind('.');
					currFilename << fname.substr(0,dotOffs) << x << fname.substr(dotOffs);
				}
				ofstream	jsonfile(currFilename.str());
				jsonfile << reply.data();
				
				fileContentsCallback( reply.data() );
				
				if( url == lastPage )
					break;
				nextPage = reply.link_header_rel( "next" );
				lastPage = reply.link_header_rel( "last" );
				if( lastPage.empty() || nextPage.empty() )
					break;
				
				url = nextPage;
				x++;
			}
			else
			{
				cerr << "Curl Error " << errcode << " downloading from '" << url << "'" << endl;
				return;
			}
		}
	}
}


string	cached_download( string url, string fname, string userName, string password )
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
		url_request	request;
		url_reply	reply;
		
		request.add_header( "User-Agent: " USER_AGENT );
		request.set_user_name( userName );
		request.set_password( password );
	
		cout << "\tFetching URL: " << url << endl;
		CURLcode	errcode = request.load( url, reply );
		if( errcode == CURLE_OK )
		{
			replyData = reply.data();
			ofstream	jsonfile(fname);
			jsonfile << replyData;
		}
		else
		{
			cerr << "Curl Error " << errcode << " downloading from '" << url << "'" << endl;
			return std::string();
		}
	}

	return replyData;
}


void	download_comments( int bugNumber, string commentsURL, string userName, string password )
{
	ifstream		downloadedcommentsjson;
	stringstream	downloadedcommentsfilename;
	downloadedcommentsfilename << "cache/" << bugNumber << "_comments_downloaded.json";
	string			commentsdata = cached_download( commentsURL, downloadedcommentsfilename.str(), userName, password );
	
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


void	print_syntax()
{
	cerr << "Syntax: bugmatic <operation> ..." << endl
			<< "\tbugmatic init" << endl
			<< "\tbugmatic clone <username> <project> [<projectUsername>]" << endl
			<< "\tbugmatic new [<title> [<body>]]" << endl
			<< "\tbugmatic list [WHERE field=value [AND field=value [AND ...]]]" << endl
			<< "\tbugmatic new-remote <username> <project> [<projectUsername>]" << endl;
}


int main( int argc, const char * argv[] )
{
	try
	{
		if( argc < 2 )
		{
			print_syntax();
			return 1;
		}
		string	operation = argv[1];

		if( operation == "init" )
		{
			mkdir("issues",0777);
			mkdir("milestones",0777);
			mkdir("users",0777);
			mkdir("cache",0777);
			
			ofstream	settingsfile("cache/bugmatic_state");
			settingsfile << "next_bug_number: " << 1 << endl;
	
			cout << "Done." << endl;
		}
		else if( operation == "list" )
		{
			filesystem::directory_iterator currFile(filesystem::path(argv[0]).parent_path() / "issues");
			for( ; currFile != filesystem::directory_iterator(); ++currFile )
			{
				filesystem::path	fpath( (*currFile).path() );
				string				fname( fpath.filename().string() );
				if( fname.length() > 0 && fname[0] == '.' )
					continue;
				ifstream	jsonFile( fpath.string() );
				string		fileData( file_contents( jsonFile ) );
				string		errMsg;
				Json		currItem = Json::parse( fileData, errMsg );
				if( errMsg.length() == 0 )
				{
					bool	skip = false;
					for( size_t x = 2; x < argc && !skip; x += 2 )
					{
						if( (x == 2 && strcasecmp(argv[x],"WHERE") == 0) || strcasecmp(argv[x],"AND") == 0 )
						{
							string condition( argv[x+1] );
							off_t pos = condition.find('=');
							string field( (pos != string::npos) ? condition.substr(0,pos) : "" );
							string value( (pos != string::npos) ? condition.substr(pos +1,string::npos) : "" );
							Json jsonField( currItem );
							off_t fieldStart = 0;
							while( true )
							{
								off_t fieldEnd = field.find('.', fieldStart);
								string currField( field.substr(fieldStart,fieldEnd) );
								jsonField = jsonField[currField];
								if( fieldEnd == string::npos )
									break;
								fieldStart = fieldEnd +1;
							}
							string currValue;
							switch( jsonField.type() )
							{
								case Json::STRING:
									currValue = jsonField.string_value();
									break;
								default:
									currValue = jsonField.dump();
							}
							if( strcasecmp(currValue.c_str(),value.c_str()) != 0 )
								skip = true;
						}
					}
					
					if( !skip )
					{
						int	bugNumber = currItem["number"].int_value();
						cout << "#" << bugNumber << ": " << currItem["title"].string_value();
						for( const Json& currLabel : currItem["labels"].array_items() )
						{
							cout << " [" << currLabel["name"].string_value() << "]";
						}
						cout << endl;
					}
				}
				else
				{
					cerr << errMsg << endl;
				}
			}
	
			cout << "Done." << endl;
		}
		else if( operation == "new" )
		{
			string		title = "New Bug";
			string		body = "Description:\nsummary.\n\nSteps to Reproduce:\n1. step\n\nExpected Result:\nexpected.\n\nActual Result:\nresult\n";
			if( argc >= 3 )
				title = argv[2];
			if( argc >= 4 )
				body = argv[3];
			
			ifstream	settingsfile("cache/bugmatic_state");
			string		settings = file_contents( settingsfile );
			off_t		searchPos = 0;
			size_t		strLen = settings.length();
			int			bugNumber = 1;
			stringstream	newsettings;
			while( true )
			{
				off_t pos = settings.find('\n', searchPos);
				if( pos == string::npos )
					pos = strLen;
				string currline = settings.substr( searchPos, pos -searchPos );
				std::pair<string,string>	setting = url_reply::header_name_and_value( currline );
				
				if( setting.first == "next_bug_number" )
				{
					bugNumber = atoi( setting.second.c_str() );
					newsettings << "next_bug_number: " << (bugNumber +1) << endl;
				}
				else
				{
					newsettings << currline << endl;
				}
				
				if( pos >= strLen )
					break;
				searchPos = pos +1;
			}
			
			Json			currItem = Json( (map<string,string>){ { "body", body }, { "number", to_string(bugNumber) }, { "title", title } } );
			stringstream	issuefilename;
			issuefilename << "issues/" << bugNumber << ".pending.json";
			ofstream		issuefile( issuefilename.str() );
			issuefile << currItem.dump();
			
			ofstream		statefile( "cache/bugmatic_state" );
			statefile << newsettings.str();
			
			cout << "Created Issue " << issuefilename.str() << "." << endl;
		}
		else if( operation == "new-remote" )
		{
			string userName = (argc > 4) ? argv[2] : argv[4];
			string url = "https://api.github.com/repos/";
			url.append(argv[2]);
			url.append("/");
			url.append(argv[3]);
			url.append("/issues");

			string postBody =	"{\n"
								"  \"title\": \"Found a bug\",\n"
								"  \"body\": \"I'm having a problem with this.\",\n"
//								"  \"assignee\": \"octocat\",\n"
//								"  \"assignees\": [\n"
//								"    {\n"
//								"      \"login\": \"uliwitness\",\n"
//								"      \"id\": 1,\n"
//								"      \"avatar_url\": \"https://github.com/images/error/octocat_happy.gif\",\n"
//								"      \"gravatar_id\": \"\",\n"
//								"      \"url\": \"https://api.github.com/users/octocat\",\n"
//								"      \"html_url\": \"https://github.com/octocat\",\n"
//								"      \"followers_url\": \"https://api.github.com/users/octocat/followers\",\n"
//								"      \"following_url\": \"https://api.github.com/users/octocat/following{/other_user}\",\n"
//								"      \"gists_url\": \"https://api.github.com/users/octocat/gists{/gist_id}\",\n"
//								"      \"starred_url\": \"https://api.github.com/users/octocat/starred{/owner}{/repo}\",\n"
//								"      \"subscriptions_url\": \"https://api.github.com/users/octocat/subscriptions\",\n"
//								"      \"organizations_url\": \"https://api.github.com/users/octocat/orgs\",\n"
//								"      \"repos_url\": \"https://api.github.com/users/octocat/repos\",\n"
//								"      \"events_url\": \"https://api.github.com/users/octocat/events{/privacy}\",\n"
//								"      \"received_events_url\": \"https://api.github.com/users/octocat/received_events\",\n"
//								"      \"type\": \"User\",\n"
//								"      \"site_admin\": false\n"
//								"    }\n"
//								"  ],\n"
//								"  \"milestone\": 1,\n"
								"  \"labels\": [\n"
								"    \"bug\"\n"
								"  ]\n"
								"}";
			
			string	password;
			char passBuf[1024] = {};

			printf( "Password for %s: ", userName.c_str() );
			scanf( "%s", passBuf );
			if( passBuf[0] == 0 )	// Seems Xcode sometimes skips the first read call. So try again in case it's Xcode's stupid console.
				scanf( "%s", passBuf );

			password = passBuf;

			url_request	request;
			url_reply	reply;
			
			request.add_header( "User-Agent: " USER_AGENT );
			request.add_header( "Content-Type: text/json" );
			request.set_user_name( userName );
			request.set_password( password );
			request.set_post_body( postBody );
			
			printf( "Post body: %zu bytes.\n", postBody.length() );
			cout << "\tFetching URL: " << url << endl;
			CURLcode	errcode = request.load( url, reply );
			if( errcode == CURLE_OK )
			{
				printf( "Status Code: %d\n", reply.status() );
			}
		}
		else if( operation == "clone" )
		{
			if( argc < 4 || argc > 5 )
			{
				print_syntax();
				return 1;
			}
			string	userName = argv[2];
			string	project = argv[3];
			string	projectUserName = userName;
			if( argc > 4 )
				projectUserName = argv[4];

			string	password;
			char passBuf[1024] = {};

			printf( "Password for %s: ", userName.c_str() );
			scanf( "%s", passBuf );
			if( passBuf[0] == 0 )	// Seems Xcode sometimes skips the first read call. So try again in case it's Xcode's stupid console.
				scanf( "%s", passBuf );

			password = passBuf;
		
			mkdir("issues",0777);
			mkdir("milestones",0777);
			mkdir("users",0777);
			mkdir("cache",0777);
			
			// TODO: Should report rate limit exceeded like documented on https://developer.github.com/v3/#rate-limiting
			
			int				nextBugNumber = 1;
			stringstream	issuesUrl;
			issuesUrl << "https://api.github.com/repos/" << projectUserName << "/" << project << "/issues?state=all&sort=created&direction=asc";
			paged_cached_download( issuesUrl.str(), "cache/issues.json", userName, password,
				[userName,password,&nextBugNumber]( string replyData )
				{
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
								download_comments( bugNumber, currItem["comments_url"].string_value(), userName, password );
							}
							stringstream	issuefilename;
							issuefilename << "issues/" << bugNumber << ".json";
							ofstream		issuefile( issuefilename.str() );
							issuefile << currItem.dump();
							
							if( bugNumber >= nextBugNumber )
								nextBugNumber = bugNumber +1;
						}
					}
					else
					{
						cerr << errMsg << endl;
					}
				} );
	
			// Write out highest bug number so we don't need to search the database to add a bug:
			ifstream	settingsfile("cache/bugmatic_state");
			string		settings = file_contents( settingsfile );
			off_t		searchPos = 0;
			size_t		strLen = settings.length();
			stringstream	newsettings;
			while( true )
			{
				off_t pos = settings.find('\n', searchPos);
				if( pos == string::npos )
					pos = strLen;
				string currline = settings.substr( searchPos, pos -searchPos );
				string	setting = url_reply::header_name( currline );
				
				if( setting == "next_bug_number" )
				{
					newsettings << "next_bug_number: " << nextBugNumber << endl;
					nextBugNumber = 0;
				}
				else
					newsettings << currline << endl;
				
				if( pos >= strLen )
					break;
				searchPos = pos +1;
			}
			
			if( nextBugNumber != 0 )
			{
				newsettings << "next_bug_number: " << nextBugNumber << endl;
			}
			
			ofstream		statefile( "cache/bugmatic_state" );
			statefile << newsettings.str();
			
			cout << "Done." << endl;
		}
		else
			print_syntax();
	}
	catch( const std::exception& err )
	{
		cerr << "Error: " << err.what() << endl;
	}
	catch( ... )
	{
		cerr << "Unknown exception" << endl;
	}
	
	return 0;
}
