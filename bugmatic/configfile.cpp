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


std::string	bugmatic::configfile::value_for_key( const std::string& inKey )
{
	return mKeys[inKey];
}


void	bugmatic::configfile::set_value_for_key( const std::string& inKey, const std::string& inValue )
{
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
			settingsfile << currPair.first << ":" << currPair.second << "\n";
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
		if( inKey && currCh == ':' )
			inKey = false;
		else if( inKey && currCh == '\n' )
			continue;
		else if( !inKey && currCh == '\n' )
		{
			mKeys[key] = value;
			key.erase();
			value.erase();
			inKey = true;
		}
		else if( inKey )
			key.append(1, currCh);
		else
			value.append(1, currCh);
		if( settingsfile.eof() )
			break;
	}
	
	return true;
}
