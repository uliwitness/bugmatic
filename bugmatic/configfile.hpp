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
	configfile( const std::string& inPath, char separator = '=' ) : mPath(inPath), mSeparator(separator) { load(); }
	~configfile() { save(); }
	
	const std::string	value_for_key( const std::string& inKey ) const;
	void				set_value_for_key( const std::string& inKey, const std::string& inValue );
	
	bool			save();
	bool			load();
	
	void			set_dirty( bool isDirty )	{ mDirty = isDirty; }

	const std::string	operator []( const std::string& inKey ) const;

protected:
	std::string							mPath;
	std::map<std::string,std::string>	mKeys;
	bool								mDirty = false;
	char								mSeparator;
};

}

#endif /* configfile_hpp */
