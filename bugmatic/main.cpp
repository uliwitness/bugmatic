//
//  main.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 04/07/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <iostream>
#include <sstream>
#include "bugmatic.hpp"
#include <unistd.h>


using namespace std;
using namespace bugmatic;


void	print_syntax()
{
	cerr << "Syntax: bugmatic <operation> ..." << endl
			<< "\tbugmatic init" << endl
			<< "\tbugmatic clone <username> <project> [<projectUsername>]" << endl
			<< "\tbugmatic new [<title> [<body>]]" << endl
			<< "\tbugmatic list [WHERE field=value [AND field=value [AND ...]]]" << endl
			<< "\tbugmatic new-remote <username> <project> [<projectUsername>]" << endl
			<< "\tbugmatic push <username> <project> [<projectUsername>]" << endl;
}


int main( int argc, const char * argv[] )
{
	try
	{
		if( argc < 2 )
		{
			print_syntax();
			return 1;
		}
		string	operation = argv[1];
		char* currDirBuf = getcwd( nullptr, 0 );
		string currDir( currDirBuf );
		free( currDirBuf );

		if( operation == "init" )
		{
			working_copy	wc( currDir );
			wc.init();
	
			cout << "Done." << endl;
		}
		else if( operation == "list" )
		{
			vector<string>	whereClauses;
			for( size_t x = 2; x < argc; x += 2 )
			{
				if( (x == 2 && strcasecmp(argv[x],"WHERE") == 0) || strcasecmp(argv[x],"AND") == 0 )
				{
					whereClauses.push_back( argv[x+1] );
				}
			}
			
			working_copy	wc( currDir );
			wc.list( whereClauses, []( issue_info currIssue )
			{
				cout << "#" << currIssue.issue_number() << ": " << currIssue.title();
				vector<label_info>	labels = currIssue.labels();
				for( const label_info& currLabel : labels )
				{
					cout << " [" << currLabel.name() << "]";
				}
				cout << endl;
			} );
			
			cout << "Done." << endl;
		}
		else if( operation == "new" )
		{
			string		title = "New Bug";
			string		body = "Description:\nsummary.\n\nSteps to Reproduce:\n1. step\n\nExpected Result:\nexpected.\n\nActual Result:\nresult\n";
			if( argc >= 3 )
				title = argv[2];
			if( argc >= 4 )
				body = argv[3];
			
			working_copy	wc( currDir );
			string issuefilename = wc.new_issue( title, body );
			
			cout << "Created Issue " << issuefilename << "." << endl;
		}
		else if( operation == "clone" )
		{
			if( argc < 4 || argc > 5 )
			{
				print_syntax();
				return 1;
			}
			string	userName = argv[2];
			string	project = argv[3];
			string	projectUserName = userName;
			if( argc > 4 )
				projectUserName = argv[4];

			string	password;
			char passBuf[1024] = {};

			printf( "Password for %s: ", userName.c_str() );
			scanf( "%s", passBuf );
			if( passBuf[0] == 0 )	// Seems Xcode sometimes skips the first read call. So try again in case it's Xcode's stupid console.
				scanf( "%s", passBuf );

			password = passBuf;
			
			working_copy	wc(currDir);
			remote			theRemote( project, projectUserName, userName, password );
			wc.clone( theRemote );
			
			cout << "Done." << endl;
		}
		else if( operation == "push" )
		{
			if( argc < 4 || argc > 5 )
			{
				print_syntax();
				return 1;
			}
			string	userName = argv[2];
			string	project = argv[3];
			string	projectUserName = userName;
			if( argc > 4 )
				projectUserName = argv[4];

			string	password;
			char passBuf[1024] = {};

			printf( "Password for %s: ", userName.c_str() );
			scanf( "%s", passBuf );
			if( passBuf[0] == 0 )	// Seems Xcode sometimes skips the first read call. So try again in case it's Xcode's stupid console.
				scanf( "%s", passBuf );

			password = passBuf;
			
			working_copy	wc(currDir);
			remote			theRemote( project, projectUserName, userName, password );
			wc.push( theRemote );
			
			cout << "Done." << endl;
		}
		else if( operation == "label" )
		{
			if( argc < 3 )
			{
				print_syntax();
				return 1;
			}

			string labelName = argv[2];
			working_copy	wc( currDir );
			int bugNumber = (argc >= 4) ? atoi(argv[3]) : wc.next_bug_number() -1;
			
			stringstream criterion;
			criterion << "number=";
			criterion << bugNumber;
			
			wc.list((std::vector<std::string>) {criterion.str()}, [labelName](issue_info currIssue)
			{
				currIssue.add_label( labelName );
			});
			
			cout << "Done." << endl;
		}
		else
			print_syntax();
	}
	catch( const std::exception& err )
	{
		cerr << "Error: " << err.what() << endl;
	}
	catch( ... )
	{
		cerr << "Unknown exception" << endl;
	}
	
	return 0;
}
