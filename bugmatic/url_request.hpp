//
//  url_request.hpp
//  bugmatic
//
//  Created by Uli Kusterer on 04/07/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#ifndef url_request_hpp
#define url_request_hpp

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>


class url_reply
{
public:
	url_reply() : mReceivedHttpStatus(0)	{}

    inline std::string					data() const			{ return mReceivedContent; }
    inline std::string					content_type() const	{ return mReceivedContentType; }
    inline unsigned int					status() const			{ return mReceivedHttpStatus; }
    inline std::vector<std::string>		headers() const			{ return mReceivedHeaders; }
	
protected:
    std::string					mReceivedContent;
    std::string					mReceivedContentType;
	std::vector<std::string>    mReceivedHeaders;
    unsigned int				mReceivedHttpStatus;

    static size_t      append_content( void* ptr, size_t size, size_t nmemb, void* stream );
    static size_t      append_header( void* ptr, size_t size, size_t nmemb, void* stream );

	friend class url_request;
};


class url_request
{
public:
    url_request() : mCURLHandle(nullptr) {}
	
    CURLcode    load( std::string inURL, url_reply& outReply );

	inline void	add_header( std::string inHeader )							{ mHeadersToSend.push_back(inHeader); }
	inline void	add_post_field( std::string inName, std::string inValue )	{ mPostData[inName] = inValue; }

protected:
    CURL*								mCURLHandle;
	std::vector<std::string>			mHeadersToSend;
	std::map<std::string,std::string>	mPostData;
};


#endif /* url_request_hpp */
