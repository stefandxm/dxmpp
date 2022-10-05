//
//  SASLMechanism_EXTERNAL.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2019
//  Copyright (c) 2019 Deus ex Machinae. All rights reserved.
//


#include <DXMPP/SASL/SASLMechanism_EXTERNAL.hpp>
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
//#include <cryptopp/md5.h>
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

        void SASL_Mechanism_EXTERNAL::Begin()
        {

            stringstream AuthXML;

            AuthXML << "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='EXTERNAL'>";
            AuthXML << "=";
            AuthXML << "</auth>";

            //std::string blaha = AuthXML.str();
            Uplink->WriteTextToSocket(AuthXML.str());
        }

        void SASL_Mechanism_EXTERNAL::Challenge(const pugi::xpath_node &challenge)
        {
        }

        bool SASL_Mechanism_EXTERNAL::Verify(const pugi::xpath_node &SuccessTag)
        {
            return true;
        }

    }
}
