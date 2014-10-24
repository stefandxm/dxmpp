//
//  SASLMechanism_PLAIN.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#include <DXMPP/SASL/SASLMechanism_PLAIN.hpp>
#include <DXMPP/pugixml/pugixml.hpp>
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
//#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/queue.h>

#include <sstream>
#include <ostream>
#include <iostream>

#include "SaslChallengeParser.hpp"

using namespace std;

namespace DXMPP
{
    namespace SASL
    {
        namespace Weak
        {
            void SASL_Mechanism_PLAIN::Begin()
            {
                std::string authid = MyJID.GetUsername();
                std::string authzid = "";
    
                byte tempbuff[1024];
                int offset= 0;
                memcpy(tempbuff+offset, authzid.c_str(), authzid.length());
                offset+=authzid.length();
                tempbuff[offset++]=0;
    
                memcpy(tempbuff+offset, authid.c_str(), authid.length());
                offset+=authid.length();
                tempbuff[offset++]=0;
    
                memcpy(tempbuff+offset, Password.c_str(), Password.length());
                offset+=Password.length();
                
                string EncodedResponse;
                CryptoPP::ArraySource ss(tempbuff, offset, true,
                                            new CryptoPP::Base64Encoder(new CryptoPP::StringSink(EncodedResponse),
                                                                        false));
                
                
                stringstream AuthXML;
                AuthXML << "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>";// << std::endl;
                AuthXML << EncodedResponse;
                AuthXML << "</auth>";
                
    
                std::string blaha = AuthXML.str();
                Uplink->WriteTextToSocket(AuthXML.str());
            }
            
            void SASL_Mechanism_PLAIN::Challenge(const pugi::xpath_node &challenge)
            {
            }
        }
    }
}
