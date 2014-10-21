//
//  SASL.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//
#ifndef DXMPP_SASL_hpp
#define DXMPP_SASL_hpp

#include "pugixml/pugixml.hpp"
#include "JID.hpp"

#include <boost/function.hpp>
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/queue.h>

#include "SaslChallengeParser.hpp"

namespace DXMPP
{
    namespace SASL
    {
        using namespace pugi;

        class SASL
        {           
        public:
                    
            typedef CryptoPP::SHA1 SHAVersion; // CryptoPP::SHA256

            enum class SASLMechanisms
            {
                None = 0,
                PLAIN   = 1,
                DIGEST_MD5  = 2,
                CRAM_MD5    = 3, // Not implemented
                SCRAM_SHA1  = 4
            };

            bool FeaturesSASL_DigestMD5;
            bool FeaturesSASL_CramMD5;
            bool FeaturesSASL_ScramSHA1;
            bool FeaturesSASL_Plain;

            SASLMechanisms ChosenSASLMechanism;

            JID MyJID;
            std::string Password;

            typedef boost::function<void (const std::string&)> RawWriter;
            RawWriter WriteTextToSocket;

            SASL(const RawWriter &Writer,
                const JID &MyJID, 
                const std::string &Password)        
                :WriteTextToSocket (Writer), MyJID(MyJID), Password(Password)
            {   
                FeaturesSASL_DigestMD5 = false;
                FeaturesSASL_ScramSHA1 = false;
                FeaturesSASL_Plain = false;
            }
            std::string SelectedNounce;


            void BeginDigestMD5();          
            void BeginScramMD5();
            void BeginScramSHA1();      
            void BeginPLAIN();

            std::string GetMD5Hex(std::string Input);           
            std::string GetHA1(std::string X, std::string nonce, std::string cnonce);
            void Challenge(const pugi::xpath_node &challenge);          
        };
    }
}

#endif