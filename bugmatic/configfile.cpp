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
		string			currSection;

		for( auto currPair : mKeys )
		{
			string key = currPair.first;
			size_t dotPos = key.find('.');
			if( dotPos != string::npos && dotPos +1 <= key.length() )
			{
				string section = key.substr(0, dotPos);
				key = key.substr(dotPos + 1);
				if( section.compare(currSection) != 0 )
				{
					currSection = section;
					settingsfile << "[" << currSection << "]\n";
				}
			}
			else if( currSection != "" )
			{
				settingsfile << "[]\n";
			}
			settingsfile << key << string(1, mSeparator) << currPair.second << "\n";
		}
	}
	
	return true;
}


bool	bugmatic::configfile::load()
{
	ifstream		settingsfile(mPath);
	bool			inKey = true;
	string			key, value, section;
	
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
				section = key.substr(1, key.length() - 2);
			}
			key = "";
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

			if( section.length() > 0 ) {
				key = section + "." + key;
			}
			
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
