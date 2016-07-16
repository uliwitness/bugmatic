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
