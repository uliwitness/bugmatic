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
#include <iomanip>
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
#include <uuid/uuid.h>
#include "json11.hpp"
#include "url_request.hpp"
#include "fake_filesystem.hpp"	// until <filesystem> becomes available.
#include "configfile.hpp"
#include "md5hash.hpp"


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


void	paged_cached_download( string url, string fname, string userName, string password, bool ignoreCache, std::function<void(string)> fileContentsCallback )
{
	ifstream	downloadedjson;
	downloadedjson.open(fname);
	if( downloadedjson.is_open() && !ignoreCache )
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


string	cached_download( string url, string fname, string userName, string password, bool ignoreCache )
{
	string		replyData;
	ifstream	downloadedjson;
	downloadedjson.open(fname);
	if( downloadedjson.is_open() && !ignoreCache )
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


string	raw_download( string url, string userName, string password )
{
	string		replyData;
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
	}
	else
	{
		cerr << "Curl Error " << errcode << " downloading from '" << url << "'" << endl;
		return std::string();
	}
	
	return replyData;
}



void	download_comments( int bugNumber, string commentsURL, string userName, string password, bool ignoreCache, int *outNextCommentNumber )
{
	string			errStr;
	stringstream	downloadedcommentsfilename;
	downloadedcommentsfilename << "cache/" << bugNumber << "_comments_downloaded.json";
	string			commentsdata = cached_download( commentsURL, downloadedcommentsfilename.str(), userName, password, ignoreCache );
	
	stringstream	issuecommentfoldername;
	issuecommentfoldername << "issues/" << bugNumber << "_comments";
	mkdir( issuecommentfoldername.str().c_str(), 0777 );
	stringstream	issuehashesfilename;
	issuehashesfilename << "cache/" << bugNumber << "_comments_hashes";
	
	configfile	commentHashes( issuehashesfilename.str() );
	
	string	errMsg;
	Json	commentsJson = Json::parse( commentsdata, errMsg );
	if( errMsg.length() == 0 )
	{
		for( const Json& currComment : commentsJson.array_items() )
		{
			stringstream	issuecommentfilename;
			issuecommentfilename << issuecommentfoldername.str() << "/" << currComment["id"].int_value() << ".json";
			string		fn = issuecommentfilename.str();
			string		fc = currComment.dump();
			int	commentNumber = currComment["id"].int_value();
			if( outNextCommentNumber && (*outNextCommentNumber) <= commentNumber )
				*outNextCommentNumber = commentNumber +1;
			
			if( filesystem::exists(fn) )
			{
				ifstream	localFile( fn );
				string		localJson( file_contents(localFile) );
				string		fileHash = hash_string( localJson );
				string		pristineHash = commentHashes.value_for_key(to_string(commentNumber));
				
				if( fileHash != pristineHash )
				{
					stringstream errMsg;
					errMsg << "Can't pull comments for issue #" << bugNumber << " because the changes to them would be overwritten.";
					throw runtime_error(errMsg.str());
				}
			}
			
			ofstream	commentfile(fn);
			commentfile << fc;
			commentHashes.set_value_for_key( to_string(commentNumber), hash_string(fc) );
		}
	}
	else
	{
		cerr << "Couldn't write comments for bug #" << bugNumber << ": " << errMsg << endl;
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


std::vector<comment_info>	issue_info::comments() const
{
	std::vector<comment_info>	commentInfos;

	filesystem::path	commentFolderPath(filesystem::path(mFilePath).parent_path() / (to_string(issue_number()) + "_comments"));
	
	// +++ TODO: need to sort filenames or we get mis-ordered comments.
	//	Could also sort commentInfos vector by date or so instead.
	filesystem::directory_iterator	currFile(commentFolderPath);
	for( ; currFile != filesystem::directory_iterator(); ++currFile )
	{
		filesystem::path	fpath( (*currFile).path() );
		string				fname( fpath.filename().string() );
		if( fname.length() > 0 && fname[0] == '.' )
			continue;
		if( fname.rfind(".json") != fname.length() -5 )
			continue;

		ifstream	jsonFile( fpath.string() );
		string		fileData( file_contents( jsonFile ) );
		string		errMsg;
		Json		currItem = Json::parse( fileData, errMsg );
		if( errMsg.length() > 0 )
		{
			stringstream errStr;
			errStr << "Can't parse comment for #" << issue_number() << ": " << errMsg;
			throw runtime_error(errStr.str());
		}
		commentInfos.push_back( comment_info( currItem, fpath.string() ) );
	}
	
	return commentInfos;
}


void	issue_info::add_comment( const std::string inBody )
{
	filesystem::path	commentFolderPath(filesystem::path(mFilePath).parent_path() / (to_string(issue_number()) + "_comments"));
	mkdir(commentFolderPath.string().c_str(), 0777);

	Json	commentJson( (map<string,Json>){ { "body", inBody } } );
	configfile	settingsfile( "cache/bugmatic_state" );
	int		commentNumber = atoi( settingsfile.value_for_key( "next_comment_number" ).c_str() );

	stringstream	commentfilename;
	commentfilename << commentFolderPath.string() << "/" << commentNumber << ".pending.json";
	ofstream		commentfile( commentfilename.str() );
	string			commentJsonStr( commentJson.dump() );
	commentfile << commentJsonStr;
	
	settingsfile.set_value_for_key( "next_comment_number", to_string(commentNumber +1) );
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


void	issue_info::save()
{
	ofstream	outFile(mFilePath);
	outFile << mIssueMetadata.dump();
}


std::string remote::url() const
{
	string url( "https://api.github.com/repos/" );
	url.append( mProjectAccount );
	url.append("/");
	url.append( mProjectName );
	
	return url;
}


void	working_copy::load_user_info()
{
	if( mWorkingCopyPath.length() > 0 )
	{
		const char *homeDir = getenv("HOME");
		if( homeDir )
		{
			configfile	gitSettings( filesystem::path(homeDir) / filesystem::path(".gitconfig"), '=' );
			mUserName = gitSettings["name"];
			mUserEmail = gitSettings["email"];
		}
	}
}


void	working_copy::init()
{
	chdir( mWorkingCopyPath.c_str() );
	
	mkdir("issues",0777);
	mkdir("milestones",0777);
	mkdir("users",0777);
	mkdir("cache",0777);
	
	ofstream	settingsfile("cache/bugmatic_state");
	settingsfile << "next_bug_number:" << 1 << endl;
	settingsfile << "next_comment_number:" << 1 << endl;
}


void	working_copy::clone( const remote& inRemote )
{
	{
		chdir( mWorkingCopyPath.c_str() );
		
		filesystem::path	wcPath(mWorkingCopyPath);
		if( filesystem::exists( (wcPath / filesystem::path("issues")).string() )
		   || filesystem::exists( (wcPath / filesystem::path("milestones")).string() )
		   || filesystem::exists( (wcPath / filesystem::path("users")).string() )
		   || filesystem::exists( (wcPath / filesystem::path("cache")).string() ) )
			throw runtime_error( "Can't clone into folder that already contains an issue database." );
		
		mkdir("issues",0777);
		mkdir("milestones",0777);
		mkdir("users",0777);
		mkdir("cache",0777);
		
		time_t now;
		time(&now);
		char dateStr[64] = {0};
		strftime( dateStr, sizeof(dateStr), "%FT%TZ", gmtime(&now) );
		string currDate( dateStr );
		
		configfile		issueHashes("cache/hashes");
		
		int				nextBugNumber = 1;
		int				nextCommentNumber = 1;
		stringstream	issuesUrl;
		issuesUrl << inRemote.url() << "/issues?state=all&sort=created&direction=asc";
		paged_cached_download( issuesUrl.str(), "cache/issues.json", inRemote.user_name(), inRemote.password(), true,
							  [inRemote,&nextBugNumber,&issueHashes,&nextCommentNumber,this]( string replyData )
							  {
								  string	errMsg;
								  Json	replyJson = Json::parse( replyData, errMsg );
								  if( errMsg.length() == 0 )
								  {
									  for( Json currItem : replyJson.array_items() )
									  {
										  int	bugNumber = currItem["number"].int_value();
										  cout << "#" << bugNumber << ": " << currItem["title"].string_value();
										  for( const Json& currLabel : currItem["labels"].array_items() )
										  {
											  cout << " [" << currLabel["name"].string_value() << "]";
										  }
										  cout << endl;
										  
										  filter_issue_body_from_github( currItem );

										  if( currItem["comments"].int_value() > 0 )
										  {
											  download_comments( bugNumber, currItem["comments_url"].string_value(), inRemote.user_name(), inRemote.password(), true, &nextCommentNumber );
										  }
										  stringstream	issuefilename;
										  issuefilename << "issues/" << bugNumber << ".json";
										  ofstream		issuefile( issuefilename.str() );
										  string			issueJsonStr( currItem.dump() );
										  issuefile << issueJsonStr;
										  
										  issueHashes.set_value_for_key( to_string(bugNumber), hash_string( issueJsonStr ) );
										  
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
		
		// Write out highest bug number etc. so we don't need to search the database to add a bug:
		ifstream		settingsfile("cache/bugmatic_state");
		string			settings = file_contents( settingsfile );
		off_t			searchPos = 0;
		size_t			strLen = settings.length();
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
				newsettings << "next_bug_number:" << nextBugNumber << endl;
				nextBugNumber = 0;
			}
			else if( setting == "next_comment_number" )
			{
				newsettings << "next_comment_number:" << nextCommentNumber << endl;
				nextCommentNumber = 0;
			}
			else if( setting == "last_synchronized_date" )
			{
				newsettings << "last_synchronized_date:" << currDate << endl;
				currDate = "";
			}
			else
				newsettings << currline << endl;
			
			if( pos >= strLen )
				break;
			searchPos = pos +1;
		}
		
		if( nextBugNumber != 0 )
		{
			newsettings << "next_bug_number:" << nextBugNumber << endl;
		}
		if( nextCommentNumber != 0 )
		{
			newsettings << "next_comment_number:" << nextCommentNumber << endl;
		}
		if( currDate.length() != 0 )
		{
			newsettings << "last_synchronized_date:" << currDate << endl;
		}
		
		ofstream		statefile( "cache/bugmatic_state" );
		statefile << newsettings.str();
	}
	
	if( mChangeHandler )
	{
		mChangeHandler( *this );
	}
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
		if( fname.rfind(".json") != fname.length() -5 )
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
	int					bugNumber = 1;
	
	{
		filesystem::path	wcPath(mWorkingCopyPath);
		filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
		ifstream			settingsfile( settingsPath.string() );
		string				settings = file_contents( settingsfile );
		off_t				searchPos = 0;
		size_t				strLen = settings.length();
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
				newsettings << "next_bug_number:" << (bugNumber +1) << endl;
			}
			else
			{
				newsettings << currline << endl;
			}
			
			if( pos >= strLen )
				break;
			searchPos = pos +1;
		}
		
		uuid_t			uuid;
		uuid_generate(uuid);
		
		uuid_string_t	uuidstring;
		uuid_unparse(uuid, uuidstring);
		
		string			uuidCppString( uuidstring );
		Json			currItem = Json( (map<string,Json>){ { "body", inBody }, { "number", bugNumber }, { "title", inTitle }, { "uuid", uuidCppString } } );
		stringstream	issuefilename;
		issuefilename << "issues/" << bugNumber << ".pending.json";
		ofstream		issuefile( (wcPath / filesystem::path(issuefilename.str())).string() );
		issuefile << currItem.dump();
		
		ofstream		statefile( (wcPath / "cache/bugmatic_state").string() );
		statefile << newsettings.str();
	}
	
	if( mChangeHandler )
	{
		mChangeHandler( *this );
	}

	return bugNumber;
}


void	working_copy::push( const remote& inRemote )
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/hashes" );
	configfile			hashesFile( settingsPath.string() );

	list( std::vector<std::string>(), [inRemote,&hashesFile,this]( issue_info currIssue )
	{
		Json	postBodyJson = currIssue.issue_json();
		string	postBody = postBodyJson.dump();
		string	postHashStr = hash_string(postBody);
		string	hashesFileEntry = hashesFile.value_for_key(to_string(currIssue.issue_number()));
		int		bugNumber = currIssue.issue_number();
		string	url = currIssue.url();
		
		if( url == "" ) // New, not yet on server?
		{
			url = inRemote.url();
			url.append("/issues");
		}
		else if( postHashStr != hashesFileEntry )	// Already on server, but changed locally.
		{
			string errMsg;
			string remoteJsonData = raw_download( url, inRemote.user_name(), inRemote.password() );
			Json remoteJson = Json::parse( remoteJsonData, errMsg );
			if( errMsg.length() == 0 )
			{
				if( remoteJson["message"].is_string() )
				{
					stringstream ss;
					ss << "GET request to " << url << " failed with Github error: " << remoteJson["message"].string_value();
					throw runtime_error( ss.str() );
				}
			}
			else
			{
				stringstream ss;
				ss << "GET request to " << url << " failed with Json parsing error: " << errMsg;
				throw runtime_error( ss.str() );
			}
			
			filter_issue_body_from_github(remoteJson);
			
			string remoteJsonDataUnpacked = remoteJson.dump();
			string remoteHash = hash_string(remoteJsonDataUnpacked);
			
			if( remoteHash != hashesFileEntry )
			{
				stringstream ss;
				ss << "Issue #" << currIssue.issue_number() << " has been changed locally and remotely. Can't yet merge issues.";
				cout << "===== local: =====" << endl << postBody << endl << "===== remote: =====" << endl << remoteJsonData << endl << "===== remote reformatted: =====" << endl << remoteJsonDataUnpacked;
				throw runtime_error( ss.str() );
			}
			
			// Only locally changed? We can push our changes upstream!
		}
		
		if( url != "" ) // Either creation or editing URL was set? Post new data!
		{
			url_request	request;
			url_reply	reply;

			filter_issue_body_for_github( postBodyJson );
			string	postBody = postBodyJson.dump();

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
					
					{
						off_t	pos = currIssue.filepath().rfind("/");
						assert( pos != string::npos );
						string newPath = currIssue.filepath().substr(0, pos+1);
						newPath.append( to_string(replyJson["number"].int_value()) );
						newPath.append(".json");
						
						filter_issue_body_from_github( replyJson );

						postBody = replyJson.dump();
						ofstream	newFile(newPath);
						newFile << postBody;

						bugNumber = replyJson["number"].int_value();
						postHashStr = hash_string(postBody);
						hashesFile.set_value_for_key(to_string(bugNumber), postHashStr);
						hashesFile.save();

						unlink( currIssue.filepath().c_str() );
					}

					if( mChangeHandler )
					{
						mChangeHandler( *this );
					}
				}
				else
				{
					stringstream ss;
					ss << "POST request to " << url << " failed with JSON error: " << errMsg;
					throw runtime_error( ss.str() );
				}
			}
			else
			{
				stringstream ss;
				ss << "POST request to " << url << " failed with curl error: " << errcode;
				throw runtime_error( ss.str() );
			}
		}
		
		for( comment_info& currComment : currIssue.comments() )
		{
			push_comment(inRemote, currIssue, currComment);
		}
	} );
}


void	working_copy::push_comment( const remote& inRemote, issue_info& currIssue, comment_info& currComment )
{
	url_request	request;
	url_reply	reply;
	
	stringstream	issuehashesfilename;
	issuehashesfilename << "cache/" << currIssue.issue_number() << "_comments_hashes";
	
	configfile	commentHashes( filesystem::path(mWorkingCopyPath) / filesystem::path(issuehashesfilename.str()) );
	
	Json	postBodyJson = currComment.comment_json();
	string	postBody = postBodyJson.dump();
	
	string url = currComment.url();
	if( url.length() == 0 )
	{
		url = currIssue.comments_url();
	}
	else
	{
		string pristineHash = commentHashes.value_for_key( to_string(currComment.comment_id()) );
		string onDiskHash = hash_string(postBody);
		
		if( pristineHash == onDiskHash ) // Unchanged? Nothing to do.
		{
			return;
		}
		
		string errMsg;
		string remoteJsonData = raw_download( url, inRemote.user_name(), inRemote.password() );
		Json remoteJson = Json::parse( remoteJsonData, errMsg );
		if( errMsg.length() == 0 )
		{
			if( remoteJson["message"].is_string() )
			{
				stringstream ss;
				ss << "GET request to " << url << " failed with Github error: " << remoteJson["message"].string_value();
				throw runtime_error( ss.str() );
			}
		}
		else
		{
			stringstream ss;
			ss << "GET request to " << url << " failed with Json parsing error: " << errMsg;
			throw runtime_error( ss.str() );
		}
		
		string remoteJsonDataUnpacked = remoteJson.dump();
		string remoteHash = hash_string(remoteJsonDataUnpacked);
		
		if( remoteHash != pristineHash )
		{
			stringstream ss;
			ss << "Comment #" << currComment.comment_id() << " of Issue #" << currIssue.issue_number() << " has been changed locally and remotely. Can't yet merge comments.";
			cout << "===== local: =====" << endl << postBody << endl << "===== remote: =====" << endl << remoteJsonData << endl << "===== remote reformatted: =====" << endl << remoteJsonDataUnpacked;
			throw runtime_error( ss.str() );
		}
	}

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
		}
		else
		{
			stringstream ss;
			ss << "POST request to " << url << " failed with Json parsing error: " << errMsg;
			throw runtime_error( ss.str() );
		}
		
		stringstream	issuecommentfoldername;
		issuecommentfoldername << "issues/" << currIssue.issue_number() << "_comments";
		mkdir( issuecommentfoldername.str().c_str(), 0777 );
		stringstream	issuecommentfilename;
		issuecommentfilename << issuecommentfoldername.str() << "/" << replyJson["id"].int_value() << ".json";

		if( currComment.is_pending() )
		{
			unlink( currComment.filepath().c_str() );
		}

		string 		replyData = replyJson.dump();
		ofstream	commentfile(filesystem::path(mWorkingCopyPath) / filesystem::path(issuecommentfilename.str()));
		commentfile << replyData;
		
		commentHashes.set_value_for_key(to_string(replyJson["id"].int_value()), hash_string(replyData));
		commentHashes.save();

		if( mChangeHandler )
		{
			mChangeHandler( *this );
		}
	}
	else
	{
		stringstream ss;
		ss << "POST request to " << url << " failed with curl error: " << errcode;
		throw runtime_error( ss.str() );
	}
}


void	working_copy::filter_issue_body_for_github( Json &replyJson )
{
	if( !replyJson["uuid"].is_null() )
	{
		string issueBody = replyJson["body"].string_value();
		const char* uuidLabel = "Bugmatic-UUID:";
		size_t uuidLabelOffset = issueBody.find(uuidLabel);
		if( uuidLabelOffset != string::npos )
		{
			size_t uuidLabelEnd = uuidLabelOffset + strlen(uuidLabel);
			uuidLabelEnd = issueBody.find_first_not_of(" \t\r\n", uuidLabelEnd);
			if( uuidLabelEnd == string::npos )
			{
				return;
			}
			size_t uuidEnd = issueBody.find_first_of( " \t\r\n", uuidLabelOffset );
			if( uuidEnd == string::npos )
			{
				uuidEnd = issueBody.length();
			}
			issueBody.replace( uuidLabelEnd, uuidEnd - uuidLabelEnd, replyJson["uuid"].string_value() );
			replace_json_field("body", issueBody, replyJson);
		}
		else
		{
			issueBody.append("\n\n");
			issueBody.append(uuidLabel);
			issueBody.append(" ");
			issueBody.append(replyJson["uuid"].string_value());
			replace_json_field("body", issueBody, replyJson);
		}
		
		delete_json_field("uuid", replyJson);
	}
}


void	working_copy::filter_issue_body_from_github( Json &replyJson )
{
	string issueBody = replyJson["body"].string_value();
	const char* uuidLabel = "Bugmatic-UUID:";
	size_t uuidLabelOffset = issueBody.find(uuidLabel);
	if( uuidLabelOffset != string::npos )
	{
		size_t uuidLabelEnd = uuidLabelOffset + strlen(uuidLabel);
		uuidLabelEnd = issueBody.find_first_not_of(" \t\r\n", uuidLabelEnd);
		if( uuidLabelEnd == string::npos )
		{
			return;
		}
		size_t uuidEnd = issueBody.find_first_of( " \t\r\n", uuidLabelOffset );
		if( uuidEnd == string::npos )
		{
			uuidEnd = issueBody.length();
		}
		string uuid = issueBody.substr( uuidLabelEnd, uuidEnd - uuidLabelEnd );
		replace_json_field("uuid", uuid, replyJson);
	}
	
	delete_json_field("comments", replyJson); // Don't consider number of attached comments changing a change on the issue itself.
	
	// Now delete any fields that are `null`. Missing or `null` shouldn't make a difference to our hash.
	map<string, Json> topLevelItems = replyJson.object_items();
	for( auto keyValuePair : topLevelItems )
	{
		if( keyValuePair.second.is_null() )
		{
			delete_json_field(keyValuePair.first, replyJson);
		}
	}
}


void	working_copy::pull( const remote& inRemote )
{
	{
		
		filesystem::path	wcPath(mWorkingCopyPath);
		filesystem::path	settingsPath( wcPath / "cache/hashes" );
		configfile			hashesFile( settingsPath.string() );
		
		chdir( mWorkingCopyPath.c_str() );

		time_t now;
		time(&now);
		char dateStr[64] = {0};
		strftime( dateStr, sizeof(dateStr), "%FT%TZ", gmtime(&now) );
		string currDate( dateStr );
		
		int				nextBugNumber = 1;
		int				nextCommentNumber = 1;
		stringstream	issuesUrl;
		issuesUrl << inRemote.url() << "/issues?state=all&sort=updated&since=" << last_synchronized_date() << "&direction=asc";
		cout << issuesUrl.str() << endl;
		paged_cached_download( issuesUrl.str(), "cache/issues.json", inRemote.user_name(), inRemote.password(), true,
			[inRemote,&nextBugNumber,&hashesFile,&nextCommentNumber,this]( string replyData )
			{
				string	errMsg;
				Json	replyJson = Json::parse( replyData, errMsg );
				if( errMsg.length() == 0 )
				{
					for( Json currItem : replyJson.array_items() )
					{
						int	bugNumber = currItem["number"].int_value();
						cout << "#" << bugNumber << ": " << currItem["title"].string_value();
						for( const Json& currLabel : currItem["labels"].array_items() )
						{
							cout << " [" << currLabel["name"].string_value() << "]";
						}
						cout << endl;
						
						filter_issue_body_from_github( currItem );

						if( currItem["comments"].int_value() > 0 )
						{
							download_comments( bugNumber, currItem["comments_url"].string_value(), inRemote.user_name(), inRemote.password(), true, &nextCommentNumber );
						}
						stringstream	issuefilename;
						issuefilename << "issues/" << bugNumber << ".json";
						
						// Already have a file of this name?
						if( filesystem::exists(filesystem::path(issuefilename.str())) )
						{
							ifstream	issueFileIn(issuefilename.str());
							string issueJsonStr = file_contents(issueFileIn);
							string fileHash = hash_string( issueJsonStr );
							string pristineHash = hashesFile.value_for_key(to_string(bugNumber));
							if( pristineHash != fileHash )
							{
								stringstream errMsg;
								errMsg << "Can't pull issue #" << bugNumber << " because the changes to it would be overwritten.";
								throw runtime_error(errMsg.str());
							}
						}
						
						ofstream		issuefile( issuefilename.str() );
						string			issueJsonStr( currItem.dump() );
						issuefile << issueJsonStr;
						
						hashesFile.set_value_for_key(to_string(bugNumber), hash_string(issueJsonStr));
						
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
		
		hashesFile.save();
		
		// Write out highest bug number so we don't need to search the database to add a bug:
		ifstream		settingsfile("cache/bugmatic_state");
		string			settings = file_contents( settingsfile );
		off_t			searchPos = 0;
		size_t			strLen = settings.length();
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
				newsettings << "next_bug_number:" << nextBugNumber << endl;
				nextBugNumber = 0;
			}
			else if( setting == "next_comment_number" )
			{
				newsettings << "next_comment_number:" << nextCommentNumber << endl;
				nextCommentNumber = 0;
			}
			else if( setting == "last_synchronized_date" )
			{
				newsettings << "last_synchronized_date:" << currDate << endl;
				currDate = "";
			}
			else
				newsettings << currline << endl;
			
			if( pos >= strLen )
				break;
			searchPos = pos +1;
		}
		
		if( nextBugNumber != 0 )
		{
			newsettings << "next_bug_number:" << nextBugNumber << endl;
		}
		if( nextCommentNumber != 0 )
		{
			newsettings << "next_comment_number:" << nextCommentNumber << endl;
		}
		if( currDate.length() != 0 )
		{
			newsettings << "last_synchronized_date:" << currDate << endl;
		}
		
		ofstream		statefile( "cache/bugmatic_state" );
		statefile << newsettings.str();
	}

	if( mChangeHandler )
	{
		mChangeHandler( *this );
	}
}


int working_copy::next_comment_number() const
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
	configfile			settingsfile( settingsPath.string() );
	return atoi( settingsfile.value_for_key( "next_comment_number" ).c_str() );
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


string	working_copy::last_synchronized_date() const
{
	filesystem::path	wcPath(mWorkingCopyPath);
	filesystem::path	settingsPath( wcPath / "cache/bugmatic_state" );
	ifstream			settingsfile( settingsPath.string() );
	string				settings = file_contents( settingsfile );
	off_t				searchPos = 0;
	size_t				strLen = settings.length();
	string				lastDate = "1970-01-01T00:00:00Z";
	while( true )
	{
		off_t pos = settings.find('\n', searchPos);
		if( pos == string::npos )
			pos = strLen;
		string currline = settings.substr( searchPos, pos -searchPos );
		std::pair<string,string>	setting = url_reply::header_name_and_value( currline );
		
		if( setting.first == "last_synchronized_date" )
		{
			lastDate = setting.second.c_str();
		}
		
		if( pos >= strLen )
			break;
		searchPos = pos +1;
	}
	
	return lastDate;
}


void	bugmatic::replace_json_field( const string& inName, const Json &inNewValue, Json &ofValue )
{
	map<string, Json> newValues = ofValue.object_items();
	
	newValues[inName] = inNewValue;
	
	ofValue = Json( newValues );
}


void	bugmatic::delete_json_field( const string& inName, Json &ofValue )
{
	map<string, Json> newValues = ofValue.object_items();
	
	newValues.erase(inName);
	
	ofValue = Json( newValues );
}
