//
//  main.cpp
//  bugmatic_tests
//
//  Created by Uli Kusterer on 19/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <iostream>
#include <vector>
#include "bugmatic.hpp"
#include "fake_filesystem.hpp"


using namespace std;
using namespace bugmatic;
using namespace fake;


int main(int argc, const char * argv[])
{
	filesystem::path	test1path( filesystem::path(argv[0]).parent_path() / "data" / "testdata1" );
	working_copy		wc( test1path.string() );
	
	cout << "note: ===== Find all uliwitness' bugs =====" << endl;
	wc.list( (std::vector<std::string>){ "assignee.login=uliwitness" } );

	cout << "note: ===== Find unclosed bugs in range 1...3 =====" << endl;
	wc.list( (std::vector<std::string>){ "closed_at=null", "number<4" } );
	
	cout << "note: ===== Done. =====" << endl;
    return 0;
}
