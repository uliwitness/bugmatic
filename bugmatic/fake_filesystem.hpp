//
//  fake_filesystem.hpp
//  bugmatic
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

/*
	Until C++ 17's filesystem class ships, provide a minimal,
	source-code-compatible version of the parts of the API
	we need.
*/

#ifndef fake_filesystem_hpp
#define fake_filesystem_hpp

#include <string>
#include <dirent.h>


namespace fake
{

namespace filesystem
{
	class path : public std::string
	{
	public:
		path() = default;
		path( const path& ) = default;
		path( path&& ) = default;
		path( const char* inStr ) { assign(inStr); }
		explicit path( const std::string inStr ) { assign(inStr); }
		
		path& operator = ( const path& ) = default;
		
		path& operator /= ( const path& );
		path operator / ( const path& );
		
		std::string	filename();
		path parent_path();
	};
	
	class directory_entry
	{
	public:
		directory_entry() = default;
		directory_entry( const directory_entry& ) = default;
		directory_entry( directory_entry&& ) = default;
		explicit directory_entry( const path& inPath ) : mPath(inPath) {}

		directory_entry& operator = ( const directory_entry& ) = default;
		bool operator == ( const directory_entry& inOther )
		{
			return mPath.compare( inOther.mPath ) == 0;
		}
		bool operator != ( const directory_entry& inOther )
		{
			return mPath.compare( inOther.mPath ) != 0;
		}
		
		path	path()	{ return mPath; }
		
	protected:
		fake::filesystem::path		mPath;

		friend class directory_iterator;
	};

	class directory_iterator
	{
	protected:
		class dir
		{
		public:
			explicit dir( DIR* inDir ) : mDir(inDir) {}
			
			dir* acquire()	{ ++mRefCount; return this; }
			void release()	{ if( --mRefCount == 0 ) delete this; }
			
			DIR* get_dir()	{ return mDir; }
			
		protected:
			~dir() {}
			
			DIR* mDir;
			size_t mRefCount = 1;
		};
		
	public:
		directory_iterator() = default;
		directory_iterator( path inPath );
		directory_iterator( const directory_iterator& inOriginal );
		~directory_iterator();
		
		directory_entry operator *() { return mEntry; }
		
		directory_iterator operator ++ ();
		
		directory_iterator& operator =( const directory_iterator& inOriginal );

		bool	operator == ( const directory_iterator& inOther );
		bool	operator != ( const directory_iterator& inOther );
		
	protected:
		directory_entry	mEntry;
		fake::filesystem::path	mPath;
		dir* mDir = nullptr;
	};
}

}

#endif /* fake_filesystem_hpp */
