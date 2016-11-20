//
//  bugmatic.hpp
//  bugmatic
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#ifndef bugmatic_hpp
#define bugmatic_hpp

#include <string>
#include <vector>
#include "json11.hpp"


namespace bugmatic
{

class user_info
{
public:
	user_info( json11::Json inJson ) : mUserMetadata(inJson) {}
	
	std::string		user_login() const	{ return mUserMetadata["login"].string_value(); }
	std::string		html_url() const	{ return mUserMetadata["html_url"].string_value(); }
	std::string		url() const			{ return mUserMetadata["url"].string_value(); }
	std::string		avatar_url() const	{ return mUserMetadata["avatar_url"].string_value(); }
	bool			site_admin() const	{ return mUserMetadata["site_admin"].bool_value(); }
	std::string		user_type() const	{ return mUserMetadata["type"].string_value(); }
	int				user_id() const		{ return mUserMetadata["id"].int_value(); }
	
protected:
	json11::Json	mUserMetadata;
};


class label_info
{
public:
	label_info( json11::Json inJson ) : mLabelMetadata(inJson) {}
	
	std::string	name() const				{ return mLabelMetadata["name"].string_value(); }
	std::string	url() const					{ return mLabelMetadata["url"].string_value(); }
	std::string	color() const				{ return mLabelMetadata["color"].string_value(); }
	bool		is_default() const			{ return mLabelMetadata["default"].bool_value(); }
	
	int			label_id() const			{ return mLabelMetadata["id"].int_value(); }
	
protected:
	json11::Json	mLabelMetadata;
};


class issue_info
{
public:
	issue_info( json11::Json inJson, std::string inPath ) : mIssueMetadata(inJson), mFilePath(inPath) {}
	
	int			issue_number() const		{ return mIssueMetadata["number"].int_value(); }
	std::string	title() const				{ return mIssueMetadata["title"].string_value(); }
	std::string	body() const				{ return mIssueMetadata["body"].string_value(); }
	std::string	closed_at() const			{ return mIssueMetadata["closed_at"].string_value(); }
	std::string	updated_at() const			{ return mIssueMetadata["updated_at"].string_value(); }
	std::string	html_url() const			{ return mIssueMetadata["html_url"].string_value(); }
	std::string	url() const					{ return mIssueMetadata["url"].string_value(); }
	std::string	repository_url() const		{ return mIssueMetadata["repository_url"].string_value(); }
	std::string	state() const				{ return mIssueMetadata["state"].string_value(); }

	std::vector<label_info>	labels() const;
	void					add_label( std::string inLabelName );
	
	std::vector<user_info>	assignees() const;
	user_info				user() const	{ return user_info( mIssueMetadata["user"] ); }
	
	int			issue_id() const			{ return mIssueMetadata["id"].int_value(); }
	
	std::string	filepath() const			{ return mFilePath; }
	bool		is_pending() const			{ off_t pos = mFilePath.rfind( ".pending.json" );return pos == mFilePath.length(); }
	
protected:
	json11::Json	issue_json()	{ return mIssueMetadata; }
	
	friend class working_copy;
	
protected:
	json11::Json	mIssueMetadata;
	std::string		mFilePath;
};


class remote
{
public:
	remote( std::string inProject, std::string inProjectAccountName, std::string inUserName, std::string inPassword ) : mProjectName(inProject), mProjectAccount(inProjectAccountName), mUserName(inUserName), mPassword(inPassword) {}
	
	std::string	project_name() const	{ return mProjectName; }
	std::string	project_account() const	{ return mProjectAccount; }
	std::string	user_name() const		{ return mUserName; }
	std::string	password() const		{ return mPassword; }
	
	std::string url() const;
	
protected:
	std::string		mProjectName;
	std::string		mProjectAccount;
	std::string		mUserName;
	std::string		mPassword;
};


class working_copy
{
public:
	explicit working_copy( std::string inWorkingCopyPath ) : mWorkingCopyPath(inWorkingCopyPath) {}
	
	void		init();
	void		clone( const remote& inRemote );
	void		list( std::vector<std::string> inWhereClauses, std::function<void(issue_info)> resultsCallback );
	int			new_issue( std::string inTitle, std::string inBody );
	void		push( const remote& inRemote );
	void		pull( const remote& inRemote );
	
	int			next_bug_number() const;
	std::string	last_synchronized_date() const;

protected:
	std::string		mWorkingCopyPath;
};


}

#endif /* bugmatic_hpp */
