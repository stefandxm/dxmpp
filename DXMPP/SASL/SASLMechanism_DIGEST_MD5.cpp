//
//  SASLMechanism_DIGEST_MD5.cpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/SASL/SASLMechanism_DIGEST_MD5.hpp>
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

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/queue.h>

#include <sstream>
#include <ostream>
#include <iostream>

#include "SaslChallengeParser.hpp"

using namespace std;
using namespace pugi;

namespace DXMPP
{
    namespace SASL
    {
        namespace Weak
        {
            void SASL_Mechanism_DigestMD5::Begin()
            {
                //ChosenSASLMechanism = SASLMechanisms::DIGEST_MD5;
                string AuthXML ="<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='DIGEST-MD5'/>";
                Uplink->WriteTextToSocket(AuthXML);
            }
            
            string SASL_Mechanism_DigestMD5::GetMD5Hex(string Input)
            {
                CryptoPP::Weak::MD5 hash;
                byte digest[ CryptoPP::Weak::MD5::DIGESTSIZE ];
                int length =CryptoPP::Weak::MD5::DIGESTSIZE;
                
                CryptoPP::HexEncoder Hexit;
                std::string TOutput;
                Hexit.Attach( new CryptoPP::StringSink( TOutput ) );
    
                hash.CalculateDigest( digest, reinterpret_cast<const unsigned char *>( Input.c_str() ), Input.length() );
                Hexit.Put( digest, length );
                Hexit.MessageEnd();
                
                boost::algorithm::to_lower(TOutput);
                return TOutput;
            }   
            
    
            string SASL_Mechanism_DigestMD5::GetHA1(string X, string nonce, string cnonce)
            {
                CryptoPP::Weak::MD5 hash;
                byte digest[ CryptoPP::Weak::MD5::DIGESTSIZE ];
                int digestlength =CryptoPP::Weak::MD5::DIGESTSIZE;
    
                // Calculatey Y
                hash.CalculateDigest( digest, reinterpret_cast<const unsigned char *>( X.c_str() ), X.length() );
                // X is now in digest           
    
                stringstream TStream;
                TStream << ":" << nonce << ":" << cnonce;
                string AuthentiationDetails = TStream.str();
                int TLen = (int)digestlength + (int)AuthentiationDetails.length();
                byte *T = new byte[TLen];
                memcpy(T, digest, digestlength);
                memcpy(T+digestlength, AuthentiationDetails.c_str(), AuthentiationDetails.length());
                hash.CalculateDigest( digest, reinterpret_cast<const unsigned char *>( T ), TLen );
                            
                CryptoPP::HexEncoder Hexit;
                std::string TOutput;
                Hexit.Attach( new CryptoPP::StringSink( TOutput ) );
                
                Hexit.Put( digest, digestlength );
                Hexit.MessageEnd();
                
                boost::algorithm::to_lower(TOutput);
                return TOutput;
            }        
            void SASL_Mechanism_DigestMD5::Challenge(const pugi::xpath_node &challenge)
            {
                string ChallengeBase64 = challenge.node().child_value();
                string DecodedChallenge = DecodeBase64(ChallengeBase64);
                
                //std::cout << "GOT CHALLENGE " << ChallengeBase64 << " decoded as " <<  DecodedChallenge << std::endl;
    
                std::map<std::string,std::string> ChallengeMap; // its like a map to teh challengeh
                            
                
                if(!ParseSASLChallenge(DecodedChallenge, ChallengeMap))
                    throw runtime_error("DXMPP: Failed to parse SASL Challenge.");
                            
                string nonce = ChallengeMap["nonce"];
                string qop = ChallengeMap["qop"];
                string cnonce = boost::lexical_cast<string>( boost::uuids::random_generator()() );
    
                string rspauth = "";
                if(ChallengeMap.find("rspauth") != ChallengeMap.end())
                    rspauth = ChallengeMap["rspauth"];
    
                if(ChallengeMap.find("cnonce") != ChallengeMap.end())
                    cnonce = ChallengeMap["cnonce"];
    
                string realm = "";// MyJID.GetDomain();
                if(ChallengeMap.find("realm") != ChallengeMap.end())
                    realm = ChallengeMap["realm"];
    
                std::string authid = MyJID.GetUsername();
                std::string authzid = MyJID.GetFullJID();
                string nc = "00000001";
    
                if(!rspauth.empty())
                {
                    Uplink->WriteTextToSocket("<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>");
                    return;
                }
    
                //cnonce= "lWThUe35I/Mykro89Cg44aks8HUCi2w0mfdrjdiz"; // hack
    
                //std::cout << "SASL Mechanism = DIGEST_MD5" << std::endl;
    
                stringstream TStream;
                std::string TOutput;
                
                // X
                TStream << MyJID.GetUsername() << ':' << realm << ':' << Password;
                string X = TStream.str();
                TStream.str("");
                TStream << "AUTHENTICATE:xmpp/" << MyJID.GetDomain();
                string A2 = TStream.str();
    
                string HA1 = GetHA1(X, nonce, cnonce);
                string HA2 = GetMD5Hex(A2);
    
                TStream.str("");
                TStream << HA1 << ':' << nonce << ':' << nc << ':'  << cnonce  << ':' << qop << ':' << HA2;
                string KD = TStream.str();
    
                string Z = GetMD5Hex(KD);
    
                TStream.str("");
                
                TStream << "username=\"" << MyJID.GetUsername() << "\""
                //<< ",realm=\"" << realm << "\""
                << ",nonce=\"" << nonce << "\""
                << ",cnonce=\"" << cnonce << "\""
                << ",nc=" << nc
                << ",qop=auth,digest-uri=\"xmpp/" << MyJID.GetDomain()<< "\""
                << ",response=" << Z
                << ",charset=utf-8";//,authzid=\"" << MyJID.GetFullJID() << "\"";
                
                string Response = TStream.str();
                Response = EncodeBase64(Response);
    
                TStream.str("");
                TStream << "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
                << Response
                << "</response>";
                
                string ResponseXML = TStream.str();
                
                Uplink->WriteTextToSocket(ResponseXML);
            }
        }
    }
}
