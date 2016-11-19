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


namespace bugmatic
{


class remote
{
public:
	remote( std::string inProject, std::string inProjectAccountName, std::string inUserName, std::string inPassword ) : mProjectName(inProject), mProjectAccount(inProjectAccountName), mUserName(inUserName), mPassword(inPassword) {}
	
	std::string	project_name() const	{ return mProjectName; }
	std::string	project_account() const	{ return mProjectAccount; }
	std::string	user_name() const		{ return mUserName; }
	std::string	password() const		{ return mPassword; }
	
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
	void		list( std::vector<std::string> inWhereClauses );
	std::string	new_issue( std::string inTitle, std::string inBody );
	void		new_issue_remote( const remote& inRemote, std::string inTitle, std::string inBody );
	
protected:
	std::string		mWorkingCopyPath;
};


}

#endif /* bugmatic_hpp */
