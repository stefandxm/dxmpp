//
//  SASLMechanism.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#include <DXMPP/SASL/SASLMechanism.hpp>
#include <pugixml/pugixml.hpp>
#include <DXMPP/JID.hpp>

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>    
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/queue.h>

#include <sstream>
#include <ostream>
#include <iostream>

#include "SaslChallengeParser.hpp"

namespace DXMPP
{
    namespace SASL
    {
        using namespace std;
        using namespace boost::asio::ip;
        using namespace boost::asio;
        using namespace pugi;   
        using namespace boost::archive::iterators;

        typedef transform_width< binary_from_base64<remove_whitespace<string::const_iterator> >, 8, 6 > Base64Decode;
        typedef base64_from_binary<transform_width<string::const_iterator,6,8> > Base64Encode;

        string SASLMechanism::DecodeBase64(string Input)
        {
            unsigned int paddChars = count(Input.begin(), Input.end(), '=');
            std::replace(Input.begin(),Input.end(),'=','A');
            string result(Base64Decode(Input.begin()), Base64Decode(Input.end()));
            result.erase(result.end()-paddChars,result.end());
            return result;
        }
        
        string SASLMechanism::EncodeBase64(string Input)
        {
            unsigned int writePaddChars = (3-Input.length()%3)%3;
            string base64(Base64Encode(Input.begin()),Base64Encode(Input.end()));
            base64.append(writePaddChars,'=');
            return base64;
        }
    }
}
