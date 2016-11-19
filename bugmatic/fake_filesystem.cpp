//
//  fake_filesystem.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include "fake_filesystem.hpp"
#include <dirent.h>


using namespace fake::filesystem;


std::string	path::filename()
{
	off_t searchStart = path::npos;
	if( length() > 0 && (*this)[length()-1] == '/' )	// Ends in slash?
		searchStart = length() -2;	// Ignore trailing slash.
	off_t pos = rfind("/", searchStart);
    if( pos != path::npos )
        return substr(pos +1);
	return "";
}


path path::parent_path()
{
	off_t searchStart = path::npos;
	if( length() > 0 && (*this)[length()-1] == '/' )	// Ends in slash?
		searchStart = length() -2;	// Ignore trailing slash.
	off_t pos = rfind("/",searchStart);
    if( pos != path::npos )
	{
        return path(substr(0,pos));
	}
	return "";
}


path& path::operator /= ( const path& inAppendee )
{
	if( length() > 0 && (*this)[length()-1] != '/' )
		append("/");
	append(inAppendee);
	
	return *this;
}


path path::operator / ( const path& inAppendee )
{
	path	newPath(*this);
	if( length() > 0 && (*this)[length()-1] != '/' )
		newPath.append("/");
	newPath.append(inAppendee);
	
	return newPath;
}


directory_iterator::directory_iterator( path inPath )
{
	mPath = inPath;
    mDir = new dir(opendir( inPath.c_str() ));
	
	// Fetch the first file entry:
	++(*this);
}


directory_iterator::directory_iterator( const directory_iterator& inOriginal )
{
	mPath = inOriginal.mPath;
	mDir = inOriginal.mDir->acquire();
}


directory_iterator::~directory_iterator()
{
	if( mDir )
		mDir->release();
}


directory_iterator& directory_iterator::operator =( const directory_iterator& inOriginal )
{
	mPath = inOriginal.mPath;
	if( mDir != inOriginal.mDir )
	{
		if( mDir )
			mDir->release();
		if( inOriginal.mDir )
			mDir = inOriginal.mDir->acquire();
		else
			mDir = nullptr;
	}
	return *this;
}

directory_iterator directory_iterator::operator ++ ()
{
	if( !mDir || mDir->get_dir() == nullptr )
	{
		mEntry.mPath = "";
		return *this;
	}
	
    struct dirent *dp = nullptr;
	do
	{
		dp = readdir( mDir->get_dir() );
		if( dp )
			mEntry.mPath = mPath / dp->d_name;
	}
	while( dp && (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) );

	if( dp == nullptr )
		mEntry.mPath = "";
	
	return *this;
}


bool	directory_iterator::operator == ( const directory_iterator& inOther )
{
	return inOther.mEntry.mPath == mEntry.mPath;
}

bool	directory_iterator::operator != ( const directory_iterator& inOther )
{
	return inOther.mEntry.mPath != mEntry.mPath;
}
