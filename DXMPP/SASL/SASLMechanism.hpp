//
//  SASLMechanism.cpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_SASL_hpp
#define DXMPP_SASL_hpp

#include <DXMPP/pugixml/pugixml.hpp>
#include <DXMPP/JID.hpp>
#include <DXMPP/SASL/SaslChallengeParser.hpp>

namespace DXMPP
{
    namespace SASL
    {
        using namespace pugi;

        class SASLMechanism
        {       
        protected:
            std::string DecodeBase64(std::string Input);
            std::string EncodeBase64(std::string Input);
            
        public:
                    
            enum class SASLMechanisms
            {
                None = 0,
                PLAIN   = 1,
                DIGEST_MD5  = 2,
                CRAM_MD5    = 3, // Not implemented
                SCRAM_SHA1  = 4
            };

            JID MyJID;
            std::string Password;

            typedef boost::function<void (const std::string&)> RawWriter;
            RawWriter WriteTextToSocket;

            SASLMechanism(const RawWriter &Writer,
                const JID &MyJID, 
                const std::string &Password)        
                :MyJID(MyJID), Password(Password), WriteTextToSocket (Writer)
            {   
            }
            std::string SelectedNounce;

            virtual void Begin() = 0;

            virtual void Challenge(const pugi::xpath_node &challenge) = 0;
        };
    }
}

#endif
