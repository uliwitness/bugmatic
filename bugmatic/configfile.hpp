//
//  configfile.hpp
//  bugmatic
//
//  Created by Uli Kusterer on 23/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#ifndef configfile_hpp
#define configfile_hpp

#include <string>
#include <map>


namespace bugmatic
{

class configfile
{
public:
	configfile( const std::string& inPath ) : mPath(inPath) { load(); }
	~configfile() { save(); }
	
	std::string		value_for_key( const std::string& inKey );
	void			set_value_for_key( const std::string& inKey, const std::string& inValue );
	
	bool			save();
	bool			load();
	
protected:
	std::string							mPath;
	std::map<std::string,std::string>	mKeys;
	bool								mDirty = false;
};

}

#endif /* configfile_hpp */
