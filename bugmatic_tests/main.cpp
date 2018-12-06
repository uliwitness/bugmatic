//
//  main.cpp
//  bugmatic_tests
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include "bugmatic.hpp"
#include "configfile.hpp"
#include "fake_filesystem.hpp"
#include "md5hash.hpp"


using namespace std;
using namespace bugmatic;
using namespace fake;


#define TEST_STR_EQUAL(a,b)		test_str_equal( (a), (b), numTests, numFailures, __FILE__, __LINE__ )
#define TEST_STR_NOT_EQUAL(a,b)	test_str_not_equal( (a), (b), numTests, numFailures, __FILE__, __LINE__ )
#define TEST_TRUE(expr)			test_true( (expr), #expr, numTests, numFailures, __FILE__, __LINE__ )


string escape_string( string msg )
{
	off_t pos = 0;
	while( (pos = msg.find('\n')) != string::npos )
	{
		msg.replace( pos, 1, "\\n" );
	}
	while( (pos = msg.find('\r')) != string::npos )
	{
		msg.replace( pos, 1, "\\r" );
	}
	while( (pos = msg.find('"')) != string::npos )
	{
		msg.replace( pos, 1, "\\\"" );
	}
	return msg;
}


void	test_str_equal( const string a, const string b, size_t& numTests, size_t &numFailures, const char* filePath, size_t lineNo )
{
	++numTests;
	
	if( a.compare(b) != 0 )
	{
		++numFailures;
		cout << "error:" << filePath << ":" << lineNo << ": failed!" << endl << a << endl;
	}
	else
		cout << "note: \"" << escape_string(a) << "\" == \"" << escape_string(b) << "\": Passed." << endl;
}


void	test_str_not_equal( const string a, const string b, size_t& numTests, size_t &numFailures, const char* filePath, size_t lineNo )
{
	++numTests;
	
	if( a.compare(b) == 0 )
	{
		++numFailures;
		cout << "error:" << filePath << ":" << lineNo << ": failed!" << endl << a << endl;
	}
	else
		cout << "note: \"" << escape_string(a) << "\" != \"" << escape_string(b) << "\": Passed." << endl;
}


void	test_true( bool inSuccess, const char* exprStr, size_t& numTests, size_t &numFailures, const char* filePath, size_t lineNo )
{
	++numTests;
	
	if( !inSuccess )
	{
		++numFailures;
		cout << "error:" << filePath << ":" << lineNo << ": failed!" << endl << exprStr << endl;
	}
	else
		cout << "note: \"" << exprStr << "\": Passed." << endl;
}


int main(int argc, const char * argv[])
{
	size_t				numTests = 0, numFailures = 0;
	
	cout << "note: ===== Test hash function =====" << endl;
	TEST_STR_EQUAL( hash_string("Same"), hash_string("Same") );
	TEST_STR_EQUAL( hash_string("Same"), "c9c90a13d8ec8fe53c6bb33fe10af6f2fe" );
	TEST_STR_NOT_EQUAL( hash_string("Same"), hash_string("Different") );
	TEST_STR_NOT_EQUAL( hash_string("Same"), hash_string("same") );
	TEST_STR_NOT_EQUAL( hash_string("long"), hash_string("Same") );
	TEST_STR_NOT_EQUAL( hash_string("same"), hash_string("Same") );
	TEST_STR_NOT_EQUAL( hash_string("Different"), hash_string("Same") );
	TEST_STR_EQUAL( hash_string(""), "d4d41d8cd98f00b204e9800998ecf8427e" );
	
	cout << endl << "note: ===== Test config files =====" << endl;
	{ ofstream deletefile("testfile.ini", ofstream::trunc); }
	{
		cout << "note: ----- setting namespaced -----" << endl;
		configfile	cf("testfile.ini");
		cf.set_value_for_key("writer.name", "Jane Eyre");
		cf.set_value_for_key("writer.email", "je@example.com");
		cf.set_value_for_key("reader.name", "Terry Pratchett");
		cf.set_value_for_key("reader.email", "gnu@example.com");
		
		cout << "note: ----- retrieving namespaced -----" << endl;
		TEST_STR_EQUAL( cf["writer.name"].c_str(), "Jane Eyre" );
		TEST_STR_EQUAL( cf["writer.email"].c_str(), "je@example.com" );
		TEST_STR_EQUAL( cf["reader.name"].c_str(), "Terry Pratchett" );
		TEST_STR_EQUAL( cf["reader.email"].c_str(), "gnu@example.com" );
	}
	{
		cout << "note: ----- reading namespaced -----" << endl;
		configfile	cf("testfile.ini");
		TEST_STR_EQUAL( cf["writer.name"].c_str(), "Jane Eyre" );
		TEST_STR_EQUAL( cf["writer.email"].c_str(), "je@example.com" );
		TEST_STR_EQUAL( cf["reader.name"].c_str(), "Terry Pratchett" );
		TEST_STR_EQUAL( cf["reader.email"].c_str(), "gnu@example.com" );
	}
	{
		cout << "note: ----- reading raw namespaced data -----" << endl;
		ifstream cis("testfile.ini");
		std::string str((std::istreambuf_iterator<char>(cis)), std::istreambuf_iterator<char>());
		TEST_STR_EQUAL(str.c_str(), "[reader]\nemail=gnu@example.com\nname=Terry Pratchett\n[writer]\nemail=je@example.com\nname=Jane Eyre\n");
	}

	cout << "note: ----- setting & retrieving plain -----" << endl;
	{ ofstream deletefile("testfile.ini", ofstream::trunc); }
	{
		configfile	cf("testfile.ini");
		cf.set_value_for_key("first", "Shonda");
		cf.set_value_for_key("second", "Rhimes");
		
		TEST_STR_EQUAL( cf["first"].c_str(), "Shonda" );
		TEST_STR_EQUAL( cf["second"].c_str(), "Rhimes" );
	}
	cout << "note: ----- reading plain -----" << endl;
	{
		configfile	cf("testfile.ini");
		TEST_STR_EQUAL( cf["first"].c_str(), "Shonda" );
		TEST_STR_EQUAL( cf["second"].c_str(), "Rhimes" );
	}
	
	cout << "note: ----- reading raw plain data -----" << endl;
	{
		ifstream cis("testfile.ini");
		std::string str((std::istreambuf_iterator<char>(cis)), std::istreambuf_iterator<char>());
		TEST_STR_EQUAL(str.c_str(), "first=Shonda\nsecond=Rhimes\n");
	}
	
	cout << "note: ----- writing raw data & reading back -----" << endl;
	{
		ofstream nufile("testfile.ini", ofstream::trunc);
		nufile << "plain=bagel\n[iconographer]\nname=Susan\nlast=Kare\n[regions]\nname=Bill\nlast=Atkinson\n";
	}
	{
		configfile	cf("testfile.ini");
		TEST_STR_EQUAL( cf["plain"].c_str(), "bagel" );
		TEST_STR_EQUAL( cf["iconographer.name"].c_str(), "Susan" );
		TEST_STR_EQUAL( cf["iconographer.last"].c_str(), "Kare" );
		TEST_STR_EQUAL( cf["regions.name"].c_str(), "Bill" );
		TEST_STR_EQUAL( cf["regions.last"].c_str(), "Atkinson" );
		cf.set_dirty(true);
	}
	{
		ifstream cis("testfile.ini");
		std::string str((std::istreambuf_iterator<char>(cis)), std::istreambuf_iterator<char>());
		TEST_STR_EQUAL(str.c_str(), "[iconographer]\nlast=Kare\nname=Susan\n[]\nplain=bagel\n[regions]\nlast=Atkinson\nname=Bill\n"); // I don't think there's an order guarantee on std::map, this test might fail on other compilers.
	}

	cout << endl << "note: ===== Find all uliwitness' bugs =====" << endl;
	filesystem::path	test1path( filesystem::path(argv[0]).parent_path() / "testdata" / "testdata1" );
	working_copy		wc( test1path.string() );

	stringstream	output1;
	wc.list( (std::vector<std::string>){ "assignee.login=uliwitness" }, [&output1]( issue_info currIssue )
	{
		output1 << currIssue.issue_number() << "," << currIssue.title();
		vector<user_info>	assignees = currIssue.assignees();
		for( const user_info& currAssignee : assignees )
			 output1 << "," << currAssignee.user_login();
		output1 << "\n";
	} );
	TEST_STR_EQUAL( output1.str(), "4,Add support for OAuth,uliwitness\n5,Should report rate limit exceeded,uliwitness\n" );

	cout << endl << "note: ===== Find unclosed bugs in range 1...3 =====" << endl;
	stringstream	output2;
	wc.list( (std::vector<std::string>){ "closed_at=null", "number<4" }, [&output2]( issue_info currIssue )
	{
		output2 << currIssue.issue_number() << "," << currIssue.title() << "," << currIssue.closed_at() << "\n";
	} );
	TEST_STR_EQUAL( output2.str(), "1,test,\n2,Found a bug,\n3,Found a bug,\n" );
	
	cout << endl << "note: ===== Create a new local bug =====" << endl;
	int	bugNumber = wc.new_issue( "This is local.", "It has issues." );
	stringstream filter;
	filter << "number=" << bugNumber;
	bool	foundOne = false;
	wc.list( (std::vector<std::string>){ filter.str() }, [&numTests,&numFailures,bugNumber,&foundOne]( issue_info currIssue )
	{
		TEST_TRUE( bugNumber == currIssue.issue_number() );
		foundOne = true;
	} );
	TEST_TRUE( foundOne );

	if( numFailures > 0 )
		cout << endl << "error: "<< numFailures << " tests of " << numTests << " failed." << endl;
	else
		cout << endl << "note: ===== All tests passed. =====" << endl;
	
    return (int)numFailures;
}
