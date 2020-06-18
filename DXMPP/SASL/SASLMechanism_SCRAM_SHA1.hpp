//
//  SASLMechanism_SCRAM_SHA1.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_SASLMechanism_SCRAM_SHA1_hpp
#define DXMPP_SASLMechanism_SCRAM_SHA1_hpp

#include "SASLMechanism.hpp"

#include <cryptopp/cryptlib.h>
#include <cryptopp/sha.h>

namespace DXMPP
{
    namespace SASL
    {
        typedef CryptoPP::SHA1 SHAVersion; // CryptoPP::SHA256
    
        class SASL_Mechanism_SCRAM_SHA1 : public SASLMechanism
        {
            std::string ServerProof;

        public:
            void Begin();
            
            SASL_Mechanism_SCRAM_SHA1(boost::shared_ptr<DXMPP::Network::AsyncTCPXMLClient> Uplink,
                const JID &MyJID, 
                const std::string &Password)        
                :
                  SASLMechanism(Uplink, MyJID, Password)
            {   
            }            
            
            void Challenge(const pugi::xpath_node &challenge);
            bool Verify(const pugi::xpath_node &SuccessTag);
        };
    }
}


#endif // DXMPP_SASLMechanism_SCRAM_SHA1_hpp
