//
//  md5hash.cpp
//  bugmatic
//
//  Created by Uli Kusterer on 23/11/2016.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include "md5hash.hpp"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <CommonCrypto/CommonCrypto.h>


using namespace std;


#define hex_char(n)		setw(2) << setfill('0') << hex << int(n)


string hash_string( const std::string& inData )
{
	stringstream issueHash;
	unsigned char hash[CC_MD5_DIGEST_LENGTH];
	CC_MD5( inData.data(), (CC_LONG)inData.size(), hash );
	issueHash << hex_char(hash[0]) << hex_char(hash[0]) << hex_char(hash[1]) << hex_char(hash[2]) << hex_char(hash[3]) << hex_char(hash[4]) << hex_char(hash[5]) << hex_char(hash[6]) << hex_char(hash[7]) << hex_char(hash[8]) << hex_char(hash[9]) << hex_char(hash[10]) << hex_char(hash[11]) << hex_char(hash[12]) << hex_char(hash[13]) << hex_char(hash[14]) << hex_char(hash[15]) << dec;
	return issueHash.str();
}
