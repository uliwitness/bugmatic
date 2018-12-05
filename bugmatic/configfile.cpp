//
//  configfile.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 23/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include "configfile.hpp"
#include <fstream>


using namespace bugmatic;
using namespace std;


const std::string	bugmatic::configfile::operator []( const std::string& inKey ) const
{
	return value_for_key(inKey);
}


const std::string	bugmatic::configfile::value_for_key( const std::string& inKey ) const
{
	auto foundValue = mKeys.find(inKey);
	if( foundValue == mKeys.end() )
	{
		return string();
	}
	return foundValue->second;
}


void	bugmatic::configfile::set_value_for_key( const std::string& inKey, const std::string& inValue )
{
	assert(inKey.find_first_of(string("\r\n") + mSeparator) == string::npos);

	mKeys[inKey] = inValue;
	mDirty = true;
}


bool	bugmatic::configfile::save()
{
	if( mDirty )
	{
		ofstream		settingsfile(mPath);

		for( auto currPair : mKeys )
		{
			settingsfile << currPair.first << string(1, mSeparator) << currPair.second << "\n";
		}
	}
	
	return true;
}


bool	bugmatic::configfile::load()
{
	ifstream		settingsfile(mPath);
	bool			inKey = true;
	string			key, value;
	
	while( true )
	{
		int	currCh = settingsfile.get();
		if( currCh == EOF )
			break;
		if( inKey && currCh == mSeparator )
			inKey = false;
		else if( inKey && currCh == '\n' )
		{
			if( key.find("[") == 0 && key.rfind("]") == key.length() -1 )
			{
				key = key.substr(1, key.length() -2) + ".";
			}
			else
			{
				key = "";
			}
			continue;
		}
		else if( !inKey && currCh == '\n' )
		{
			size_t actualStart = key.find_first_not_of("\t\r\n ");
			if( actualStart == string::npos ) actualStart = 0;
			
			size_t actualEnd = key.find_last_not_of("\t\r\n ");
			if( actualEnd == string::npos ) actualEnd = key.length(); else ++actualEnd;
			
			key = key.substr( actualStart, actualEnd - actualStart );

			actualStart = value.find_first_not_of("\t\r\n ");
			if( actualStart == string::npos ) actualStart = 0;
			
			actualEnd = value.find_last_not_of("\t\r\n ");
			if( actualEnd == string::npos ) actualEnd = key.length(); else ++actualEnd;
			
			value = value.substr( actualStart, actualEnd - actualStart );

			mKeys[key] = value;
			key.erase();
			value.erase();
			inKey = true;
		}
		else if( inKey )
		{
			key.append(1, currCh);
		}
		else
		{
			value.append(1, currCh);
		}
		if( settingsfile.eof() )
			break;
	}
	
	return true;
}
