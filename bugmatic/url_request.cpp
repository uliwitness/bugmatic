//
//  url_request.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 04/07/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include "url_request.hpp"
#include <iostream>


CURLcode url_request::load( std::string url, url_reply& outReply )
{
    // clear things ready for our 'fetch'
    outReply.mReceivedHttpStatus = 0;
	outReply.mReceivedContent.clear();
    outReply.mReceivedHeaders.clear();

	mCURLHandle = curl_easy_init();

    // set our callbacks
    curl_easy_setopt( mCURLHandle, CURLOPT_WRITEFUNCTION, url_reply::append_content );
    curl_easy_setopt( mCURLHandle, CURLOPT_HEADERFUNCTION, url_reply::append_header );
    curl_easy_setopt( mCURLHandle, CURLOPT_WRITEDATA, &outReply );
    curl_easy_setopt( mCURLHandle, CURLOPT_WRITEHEADER, &outReply );

    // set the URL we want
    curl_easy_setopt( mCURLHandle, CURLOPT_URL, url.c_str() );

    struct curl_slist *chunk = NULL;

	for( const std::string& currStr : mHeadersToSend )
		chunk = curl_slist_append(chunk, currStr.c_str() );

    CURLcode	res = curl_easy_setopt( mCURLHandle, CURLOPT_HTTPHEADER, chunk );
	if( res != CURLE_OK )
		return res;

	if( mPostData.size() > 0 )
	{
		std::string		postData;
		bool			isFirst = true;
		for( std::pair<std::string,std::string> currField : mPostData )
		{
			if( isFirst )
				isFirst = false;
			else
				postData.append( 1, '&' );
			postData.append( currField.first );
			postData.append( 1, '=' );
			postData.append( currField.second );
		}
		curl_easy_setopt( mCURLHandle, CURLOPT_POSTFIELDS, postData.c_str() );
	}
	
    // go get 'em, tiger
    CURLcode curlErr = curl_easy_perform( mCURLHandle );
    if( curlErr == CURLE_OK )
	{
        // assuming everything is ok, get the content type and status code
        char* content_type = NULL;
        if( curl_easy_getinfo( mCURLHandle, CURLINFO_CONTENT_TYPE, &content_type ) == CURLE_OK )
            outReply.mReceivedContentType = std::string(content_type);

        unsigned int http_code = 0;
        if( curl_easy_getinfo( mCURLHandle, CURLINFO_RESPONSE_CODE, &http_code ) == CURLE_OK )
            outReply.mReceivedHttpStatus = http_code;
    }
	
	curl_slist_free_all(chunk);
	
	curl_easy_cleanup( mCURLHandle );
	
    return curlErr;
}


size_t url_reply::append_content( void* ptr, size_t size, size_t nmemb, void* stream )
{
	url_reply* handle = (url_reply*)stream;
	size_t data_size = size * nmemb;
    if( handle != NULL )
	{
        handle->mReceivedContent.append( (char *)ptr, data_size );
    }
	return data_size;
}


size_t url_reply::append_header( void* ptr, size_t size, size_t nmemb, void* stream )
{
    url_reply* handle = (url_reply*)stream;
	size_t data_size = size * nmemb;
    if( handle != NULL )
	{
        std::string header_line( (char *)ptr, data_size );
        handle->mReceivedHeaders.push_back( header_line );
    }
    return data_size;
}


std::map<std::string,std::string>	url_reply::header_map() const
{
	std::map<std::string,std::string>	outMap;
	
	for( const std::string& currHeader : mReceivedHeaders )
	{
		std::pair<std::string,std::string>	splitHeader = header_name_and_value( currHeader );
		outMap.insert( splitHeader );
	}
	
	return outMap;
}


std::string	url_reply::link_header_rel( std::string relValue ) const
{
	std::map<std::string,std::string>	headers = header_map();
	std::vector<std::string>	links = url_reply::header_list_items( headers["Link"] );
	for( const std::string& currLink : links )
	{
		std::string							url;
		std::map<std::string,std::string>	attributes = url_reply::list_item_attributes( currLink, url );
		if( attributes["rel"].compare(relValue) == 0 )
			return url;
	}
	
	return std::string();
}


std::string	url_reply::header_name( std::string inHeaderLine )
{
	off_t pos = inHeaderLine.find(':');
	if( pos != std::string::npos )
		return inHeaderLine.substr(0,pos);
	else
		return std::string();
}


std::pair<std::string,std::string>	url_reply::header_name_and_value( std::string inHeaderLine )
{
	std::pair<std::string,std::string>	theResult;
	off_t pos = inHeaderLine.find(':');
	if( pos != std::string::npos )
	{
		theResult.first = inHeaderLine.substr(0,pos);
		off_t	endPos = inHeaderLine.find_last_not_of( '\n' );
		off_t	endPos2 = inHeaderLine.find_last_not_of( '\r' );
		if( endPos > endPos2 )
			endPos = endPos2;
		theResult.second = inHeaderLine.substr(pos +1, endPos -(pos +1));
	}
	else
		theResult.second = inHeaderLine;
	return theResult;
}


std::vector<std::string>	url_reply::header_list_items( std::string inHeaderLine )
{
	std::vector<std::string>	theResult;
	off_t						searchPos = 0;
	while( true )
	{
		off_t pos = inHeaderLine.find(',', searchPos);
		theResult.push_back( inHeaderLine.substr(searchPos,pos -searchPos) );
		if( pos == std::string::npos )
			break;
		searchPos = pos +1;
	}
	return theResult;
}


std::map<std::string,std::string>	url_reply::list_item_attributes( std::string inHeaderLine, std::string& outFirstItem )
{
	std::map<std::string,std::string>	theResult;
	off_t								searchPos = inHeaderLine.find_first_not_of( ' ', 0 );
	off_t								pos = inHeaderLine.find(';', searchPos);
	outFirstItem = inHeaderLine.substr(searchPos,pos -searchPos);
	if( outFirstItem.length() >= 2 && outFirstItem[0] == '<' && outFirstItem[outFirstItem.length() -1] == '>' )
		outFirstItem = outFirstItem.substr( 1, outFirstItem.length() -2 );
	if( pos != std::string::npos )
	{
		searchPos = inHeaderLine.find_first_not_of( ' ', pos +1 );
		while( true )
		{
			pos = inHeaderLine.find('=', searchPos);
			std::string		key = inHeaderLine.substr(searchPos,pos -searchPos);
			searchPos = inHeaderLine.find_first_not_of( ' ', pos +1 );
			off_t			nextAttrPos = inHeaderLine.find(';', searchPos);
			std::string		val = inHeaderLine.substr(searchPos,nextAttrPos -searchPos);
			if( val.length() >= 2 && val[0] == '"' && val[val.length() -1] == '"' )
				val = val.substr( 1, val.length() -2 );
			theResult[key] = val;
			if( pos == std::string::npos )
				break;
			searchPos = pos +1;
		}
	}
	return theResult;
}

