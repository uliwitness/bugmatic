//
//  bugmatic.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include "bugmatic.hpp"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <sys/stat.h>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include "json11.hpp"
#include "url_request.hpp"
#include "fake_filesystem.hpp"	// until <filesystem> becomes available.


#define USER_AGENT		"bugmatic/0.1"


using Json = json11::Json;
using namespace std;
using namespace fake;
using namespace bugmatic;


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


std::vector<label_info>	issue_info::labels() const
{
	std::vector<label_info>	labelInfos;
	Json					labelsJson( mIssueMetadata["labels"] );
	
	for( const Json& currItem : labelsJson.array_items() )
	{
		labelInfos.push_back( label_info( currItem ) );
	}
	
	return labelInfos;
}


void	issue_info::add_label( std::string inLabelName )
{
	Json::object	fields( mIssueMetadata.object_items() );
	Json::object	newObject;
	Json::array		labelList;
	
	for( auto currField : fields )
	{
		if( currField.first == "labels" )
			labelList = currField.second.array_items();
		else
			newObject.insert( currField );
	}
	
	Json	labelJson((map<string,string>){ {"name", inLabelName} });
	labelList.push_back( labelJson );
	newObject["labels"] = Json(labelList);

	mIssueMetadata = Json( newObject );
	
	ofstream	outputFile( filepath() );
	outputFile << mIssueMetadata.dump();
}


std::vector<user_info>	issue_info::assignees() const
{
	std::vector<user_info>	userInfos;
	Json					assigneesJson( mIssueMetadata["assignees"] );
	
	for( const Json& currItem : assigneesJson.array_items() )
	{
		userInfos.push_back( user_info( currItem ) );
	}
	
	return userInfos;
}


std::string remote::url() const
{
	string url( "https://api.github.com/repos/" );
	url.append( mProjectAccount );
	url.append("/");
	url.append( mProjectName );
	
	return url;
}


void	working_copy::init()
{
	chdir( mWorkingCopyPath.c_str() );
	
	mkdir("issues",0777);
	mkdir("milestones",0777);
	mkdir("users",0777);
	mkdir("cache",0777);
	
	ofstream	settingsfile("cache/bugmatic_state");
	settingsfile << "next_bug_number: " << 1 << endl;
}


void	working_copy::clone( const remote& inRemote )
{
	chdir( mWorkingCopyPath.c_str() );

	mkdir("issues",0777);
	mkdir("milestones",0777);
	mkdir("users",0777);
	mkdir("cache",0777);
	
	int				nextBugNumber = 1;
	stringstream	issuesUrl;
	issuesUrl << inRemote.url() << "/issues?state=all&sort=created&direction=asc";
	paged_cached_download( issuesUrl.str(), "cache/issues.json", inRemote.user_name(), inRemote.password(),
		[inRemote,&nextBugNumber]( string replyData )
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
						download_comments( bugNumber, currItem["comments_url"].string_value(), inRemote.user_name(), inRemote.password() );
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
				stringstream ss("JSON parser failed: ");
				ss << errMsg;
				throw runtime_error(ss.str());
			}
		} );

	// Write out highest bug number so we don't need to search the database to add a bug:
	ifstream		settingsfile("cache/bugmatic_state");
	string			settings = file_contents( settingsfile );
	off_t			searchPos = 0;
	size_t			strLen = settings.length();
	stringstream	newsettings;
	int				lastSynchedBugNumber = nextBugNumber -1;
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
		else if( setting == "last_synchronized_bug_number" )
		{
			newsettings << "last_synchronized_bug_number: " << lastSynchedBugNumber << endl;
			lastSynchedBugNumber = 0;
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
	else if( lastSynchedBugNumber != 0 )
	{
		newsettings << "last_synchronized_bug_number: " << lastSynchedBugNumber << endl;
	}
	
	ofstream		statefile( "cache/bugmatic_state" );
	statefile << newsettings.str();
}


void	working_copy::list( std::vector<std::string> inWhereClauses, std::function<void(issue_info)> resultsCallback )
{
	filesystem::path				wcPath(mWorkingCopyPath);
	wcPath /= "issues";
	filesystem::directory_iterator	currFile(wcPath);
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
			for( string condition : inWhereClauses )
			{
				enum {
					EQUAL,
					GREATER,
					LESS
				} operation = EQUAL;
				off_t pos = condition.find('=');
				if( pos == string::npos )
				{
					pos = condition.find('>');
					if( pos != string::npos )
						operation = GREATER;
					else
					{
						pos = condition.find('<');
						if( pos != string::npos )
							operation = LESS;
						else
						{
							stringstream	ss("Can't find operation in WHERE clause \"");
							ss << condition;
							ss << "\"";
							throw runtime_error( ss.str() );
						}
					}
				}
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
						if( operation == EQUAL && strcasecmp(currValue.c_str(),value.c_str()) != 0 )
							skip = true;
						else if( operation == LESS && strcasecmp(currValue.c_str(),value.c_str()) >= 0 )
							skip = true;
						else if( operation == GREATER && strcasecmp(currValue.c_str(),value.c_str()) <= 0 )
							skip = true;
						break;
					case Json::NUMBER:
					{
						double currNumber = jsonField.number_value();
						double number = strtod( value.c_str(), nullptr );
						if( operation == EQUAL && fabs(currNumber - number) >= 0.0001 )
							skip = true;
						else if( operation == LESS && currNumber >= number )
							skip = true;
						else if( operation == GREATER && currNumber <= number )
							skip = true;
						break;
					}
					default:
						currValue = jsonField.dump();
						if( operation == EQUAL && strcasecmp(currValue.c_str(),value.c_str()) != 0 )
							skip = true;
						else if( operation == LESS && strcasecmp(currValue.c_str(),value.c_str()) >= 0 )
							skip = true;
						else if( operation == GREATER && strcasecmp(currValue.c_str(),value.c_str()) <= 0 )
							skip = true;
				}
				
				if( skip )
					break;	// No use checking other conditions if we've already failed.
			}
			
			if( !skip )
			{
				resultsCallback(issue_info(currItem,fpath.string()));
			}
		}
		else
		{
			stringstream ss( "JSON Parser failed: " );
			ss << errMsg;
			throw runtime_error( ss.str() );
		}
	}
}


int	working_copy::new_issue( std::string inTitle, std::string inBody )
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
	ifstream			settingsfile( settingsPath.string() );
	string				settings = file_contents( settingsfile );
	off_t				searchPos = 0;
	size_t				strLen = settings.length();
	int					bugNumber = 1;
	stringstream		newsettings;
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
	
	Json			currItem = Json( (map<string,Json>){ { "body", inBody }, { "number", bugNumber }, { "title", inTitle } } );
	stringstream	issuefilename;
	issuefilename << "issues/" << bugNumber << ".pending.json";
	ofstream		issuefile( (wcPath / filesystem::path(issuefilename.str())).string() );
	issuefile << currItem.dump();
	
	ofstream		statefile( (wcPath / "cache/bugmatic_state").string() );
	statefile << newsettings.str();
	
	return bugNumber;
}


void	working_copy::push( const remote& inRemote )
{
	list( (std::vector<std::string>){ "url=null" }, [inRemote]( issue_info currIssue )
	{
		string url( inRemote.url() );
		url.append("/issues");

		Json		theLabels = Json( (Json::array){ "test-bug" } );
		Json		postBodyJson = currIssue.issue_json();
		string postBody = postBodyJson.dump();
		
		url_request	request;
		url_reply	reply;
		
		request.add_header( "User-Agent: " USER_AGENT );
		request.add_header( "Content-Type: text/json" );
		request.set_user_name( inRemote.user_name() );
		request.set_password( inRemote.password() );
		request.set_post_body( postBody );
		
		CURLcode	errcode = request.load( url, reply );
		if( errcode == CURLE_OK )
		{
			if( reply.status() < 200 || reply.status() >= 300 )
			{
				stringstream ss;
				ss << "POST request to " << url << " failed with HTTP error: " << reply.status();
				throw runtime_error( ss.str() );
			}
			
			string	errMsg;
			Json	replyJson = Json::parse( reply.data(), errMsg );
			if( errMsg.length() == 0 )
			{
				if( replyJson["message"].is_string() )
				{
					stringstream ss;
					ss << "POST request to " << url << " failed with Github error: " << replyJson["message"].string_value();
					throw runtime_error( ss.str() );
				}
				
				off_t	pos = currIssue.filepath().rfind("/");
				assert( pos != string::npos );
				string newPath = currIssue.filepath().substr(0,pos+1);
				newPath.append( to_string(replyJson["number"].int_value()) );
				newPath.append(".json");
				
				ofstream	newFile(newPath);
				newFile << replyJson.dump();
				
				unlink( currIssue.filepath().c_str() );
			}
		}
		else
		{
			stringstream ss;
			ss << "POST request to " << url << " failed with curl error: " << errcode;
			throw runtime_error( ss.str() );
		}
	} );
}


int	working_copy::next_bug_number() const
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
	ifstream			settingsfile( settingsPath.string() );
	string				settings = file_contents( settingsfile );
	off_t				searchPos = 0;
	size_t				strLen = settings.length();
	int					bugNumber = 1;
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
		}
		
		if( pos >= strLen )
			break;
		searchPos = pos +1;
	}
	
	return bugNumber;
}


int	working_copy::last_synchronized_bug_number() const
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
	ifstream			settingsfile( settingsPath.string() );
	string				settings = file_contents( settingsfile );
	off_t				searchPos = 0;
	size_t				strLen = settings.length();
	int					bugNumber = 0;
	while( true )
	{
		off_t pos = settings.find('\n', searchPos);
		if( pos == string::npos )
			pos = strLen;
		string currline = settings.substr( searchPos, pos -searchPos );
		std::pair<string,string>	setting = url_reply::header_name_and_value( currline );
		
		if( setting.first == "last_synchronized_bug_number" )
		{
			bugNumber = atoi( setting.second.c_str() );
		}
		
		if( pos >= strLen )
			break;
		searchPos = pos +1;
	}
	
	return bugNumber;
}



