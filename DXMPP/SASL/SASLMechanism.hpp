//
//  SASLMechanism.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_SASL_hpp
#define DXMPP_SASL_hpp

#include <DXMPP/pugixml/pugixml.hpp>
#include <DXMPP/JID.hpp>
#include <DXMPP/SASL/SaslChallengeParser.hpp>
#include <DXMPP/Network/AsyncTCPXMLClient.hpp>

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
            DXMPP::Network::AsyncTCPXMLClient *Uplink;

            SASLMechanism(DXMPP::Network::AsyncTCPXMLClient *Uplink,
                const JID &MyJID, 
                const std::string &Password)        
                :MyJID(MyJID), Password(Password), Uplink (Uplink)
            {   
            }
            std::string SelectedNounce;

            virtual void Begin() = 0;
            virtual void Challenge(const pugi::xpath_node &ChallengeTag) = 0;
            virtual bool Verify(const pugi::xpath_node &SuccessTag) = 0;
        };
    }
}

#endif
