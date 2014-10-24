//
//  SASLMechanism_SCRAM_SHA1.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/SASL/SASLMechanism_SCRAM_SHA1.hpp>
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
        void SASL_Mechanism_SCRAM_SHA1::Begin()
        {
            stringstream TStream;
            SelectedNounce = boost::lexical_cast<string>( boost::uuids::random_generator()() );
            TStream << "n,,n=" << MyJID.GetUsername() << ",r=" << SelectedNounce;

            string Request = TStream.str();
            Request = EncodeBase64(Request);
            TStream.str("");
            TStream <<  "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='SCRAM-SHA-1'>"
            << Request
            << "</auth>";
            string AuthXML = TStream.str();
            Uplink->WriteTextToSocket(AuthXML);
        }

        void SASL_Mechanism_SCRAM_SHA1::Challenge(const pugi::xpath_node &challenge)
        {
            string ChallengeBase64 = challenge.node().child_value();
            string DecodedChallenge = DecodeBase64(ChallengeBase64);
            std::map<std::string,std::string> ChallengeMap; // its like a map to teh challengeh

            if(!ParseSASLChallenge(DecodedChallenge, ChallengeMap))
                throw runtime_error("DXMPP: Failed to parse SASL Challenge.");

            string r = ChallengeMap["r"]; // my nonce+ servers nonce
            string s = ChallengeMap["s"]; // salt
            string i = ChallengeMap["i"]; // iterations to apply hash function
            int NrIterations = boost::lexical_cast<int> (i);

            string n = MyJID.GetUsername();

            byte SaltBytes[1024];

            CryptoPP::Base64Decoder decoder;
            decoder.Put((byte*)s.c_str(), s.length());
            decoder.MessageEnd();

            int SaltLength =decoder.Get(SaltBytes, 1024-4);
            SaltBytes[SaltLength++] = 0;
            SaltBytes[SaltLength++] = 0;
            SaltBytes[SaltLength++] = 0;
            SaltBytes[SaltLength++] = 1;

            string c = "biws";
            string p= ""; // proof!! calcualte!!


            byte Result[ SHAVersion::DIGESTSIZE ];
            byte tmp[ SHAVersion::DIGESTSIZE ];
            byte ClientKey[ SHAVersion::DIGESTSIZE ];
            byte StoredKey[ SHAVersion::DIGESTSIZE ];
            byte Previous[ SHAVersion::DIGESTSIZE ];
            byte ClientSignature[ SHAVersion::DIGESTSIZE ];
            byte ClientProof[ SHAVersion::DIGESTSIZE ];
            byte ServerKey[ SHAVersion::DIGESTSIZE ];
            byte ServerSignature[SHAVersion::DIGESTSIZE ];

            int digestlength =SHAVersion::DIGESTSIZE;

            // Calculate Result
            CryptoPP::HMAC< SHAVersion > hmacFromPassword((byte*)Password.c_str(), Password.length());
            hmacFromPassword.CalculateDigest( Result, SaltBytes, SaltLength);

            memcpy(Previous, Result, digestlength);
            for(int i = 1; i < NrIterations; i++)
            {
                hmacFromPassword.CalculateDigest( tmp, Previous, digestlength);
                for( int j = 0; j < digestlength; j++)
                {
                    Result[j] = Result[j]^tmp[j];
                    Previous[j] = tmp[j];
                }
            }

            // Result is now salted password
            CryptoPP::HMAC< SHAVersion > hmacFromSaltedPassword(Result, digestlength);
            hmacFromSaltedPassword.CalculateDigest( ClientKey, (byte*)"Client Key", strlen("Client Key"));

            SHAVersion hash;
            hash.CalculateDigest( StoredKey, ClientKey, digestlength);

            stringstream TStream;
            TStream << "n=" << n << ",r=" << SelectedNounce;
            TStream << "," << DecodedChallenge;
            TStream << ",c=" << c;
            TStream << ",r=" << r;

            string AuthMessage = TStream.str();

            CryptoPP::HMAC< SHAVersion > hmacFromStoredKey(StoredKey, digestlength);
            hmacFromStoredKey.CalculateDigest( ClientSignature, (byte*)AuthMessage.c_str(), AuthMessage.length());

            for(int i = 0; i < digestlength; i++)
            {
                ClientProof[i] = ClientKey[i] ^ ClientSignature[i];
            }

            // Prepare an HMAC SHA-1 digester using the salted password bytes as the key.
            hmacFromSaltedPassword.CalculateDigest( ServerKey, (byte*)"Server Key", strlen("Server Key"));

            CryptoPP::HMAC< SHAVersion > hmacFromServerKey(ServerKey, digestlength);
            hmacFromServerKey.CalculateDigest( ServerSignature, (byte*)AuthMessage.c_str(), AuthMessage.length());

            // nu har vi räknat ut alla jävla saker vi egentligen behöver..
            TStream.str("");
            // convert something into p.. i guess client proof to base 64?
            CryptoPP::ArraySource(ClientProof, digestlength, true,
                                        new CryptoPP::Base64Encoder(new CryptoPP::StringSink(p)));

            TStream << "c=" << c << ",r=" << r << ",p=" << p;
            string Response = TStream.str();

            string Base64EncodedResponse = EncodeBase64(Response);

            TStream.str("");
            TStream << "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>";
            TStream <<Base64EncodedResponse;
            TStream << "</response>";
            string ResponseXML = TStream.str();
            Uplink->WriteTextToSocket(ResponseXML);
        }
    }
}
