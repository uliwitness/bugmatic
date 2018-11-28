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

void	replace_json_field( const std::string& inName, const json11::Json &inNewValue, json11::Json &ofValue );
void	delete_json_field( const std::string& inName, json11::Json &ofValue );

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


class comment_info
{
public:
	comment_info( json11::Json inJson ) : mCommentMetadata(inJson) {}
	
	std::string	body() const				{ return mCommentMetadata["body"].string_value(); }
	std::string	created_at() const			{ return mCommentMetadata["created_at"].string_value(); }
	std::string	updated_at() const			{ return mCommentMetadata["updated_at"].string_value(); }
	std::string	html_url() const			{ return mCommentMetadata["html_url"].string_value(); }
	std::string	url() const					{ return mCommentMetadata["url"].string_value(); }
	std::string	issue_url() const			{ return mCommentMetadata["issue_url"].string_value(); }
	user_info	user() const				{ return user_info( mCommentMetadata["user"] ); }

	int			comment_id() const			{ return mCommentMetadata["id"].int_value(); }
	
	json11::Json	comment_json()			{ return mCommentMetadata; }
	
protected:
	json11::Json	mCommentMetadata;
};


class issue_info
{
public:
	explicit issue_info( json11::Json inJson = json11::Json(), std::string inPath = "" ) : mIssueMetadata(inJson), mFilePath(inPath) {}
	issue_info( const issue_info& inOriginal ) : mIssueMetadata(inOriginal.mIssueMetadata), mFilePath(inOriginal.mFilePath) {}
	
	int			issue_number() const		{ return mIssueMetadata["number"].int_value(); }
	std::string	title() const				{ return mIssueMetadata["title"].string_value(); }
	void		set_title( std::string inTitle )	{ replace_json_field( "title", json11::Json(inTitle), mIssueMetadata ); }
	std::string	body() const				{ return mIssueMetadata["body"].string_value(); }
	void		set_body( std::string inBody )		{ replace_json_field( "body", json11::Json(inBody), mIssueMetadata ); }
	std::string	created_at() const			{ return mIssueMetadata["created_at"].string_value(); }
	std::string	closed_at() const			{ return mIssueMetadata["closed_at"].string_value(); }
	std::string	updated_at() const			{ return mIssueMetadata["updated_at"].string_value(); }
	std::string	html_url() const			{ return mIssueMetadata["html_url"].string_value(); }
	std::string	url() const					{ return mIssueMetadata["url"].string_value(); }
	std::string	uuid() const				{ return mIssueMetadata["uuid"].string_value(); }
	void		set_uuid( std::string inUUID )	{ replace_json_field( "uuid", json11::Json(inUUID), mIssueMetadata ); }
	std::string	repository_url() const		{ return mIssueMetadata["repository_url"].string_value(); }
	std::string	comments_url() const		{ return mIssueMetadata["comments_url"].string_value(); }
	std::string	state() const				{ return mIssueMetadata["state"].string_value(); }

	std::vector<label_info>	labels() const;
	void					add_label( std::string inLabelName );

	std::vector<comment_info>	comments() const;
	void						add_comment( const std::string inBody );
	
	std::vector<user_info>	assignees() const;
	user_info				user() const	{ return user_info( mIssueMetadata["user"] ); }
	
	int			issue_id() const			{ return mIssueMetadata["id"].int_value(); }
	
	std::string	filepath() const			{ return mFilePath; }
	bool		is_pending() const			{ off_t pos = mFilePath.rfind( ".pending.json" );return pos == mFilePath.length(); }
	
	issue_info & operator =( const issue_info& inOriginal ) { mIssueMetadata = inOriginal.mIssueMetadata; mFilePath= inOriginal.mFilePath; return *this; }

	
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
	explicit working_copy( std::string inWorkingCopyPath = "" ) : mWorkingCopyPath(inWorkingCopyPath) {}
	
	void		set_path( std::string inWorkingCopyPath )	{ mWorkingCopyPath = inWorkingCopyPath; }
	std::string	path()										{ return mWorkingCopyPath; }
	
	void		init();
	void		clone( const remote& inRemote );
	void		list( std::vector<std::string> inWhereClauses, std::function<void(issue_info)> resultsCallback );
	int			new_issue( std::string inTitle, std::string inBody );
	void		push( const remote& inRemote );
	void		pull( const remote& inRemote );
	
	int			next_bug_number() const;
	int			next_comment_number() const;
	std::string	last_synchronized_date() const;
	
	void		set_change_handler( std::function<void(working_copy&)> inHandler ) { mChangeHandler = inHandler; }

protected:
	void		filter_issue_body_from_github( json11::Json &replyJson );
	void		filter_issue_body_for_github( json11::Json &replyJson );

	void		push_comment( const remote& inRemote, issue_info& currIssue, comment_info& currComment );
	
	std::string		mWorkingCopyPath;
	std::function<void(working_copy&)> mChangeHandler;
};

}

#endif /* bugmatic_hpp */
