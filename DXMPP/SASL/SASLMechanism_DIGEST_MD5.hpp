//
//  SASLMechanism_DIGEST_MD5.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_SASLMechanism_DIGESTMD5_hpp
#define DXMPP_SASLMechanism_DIGESTMD5_hpp

#include "SASLMechanism.hpp"

namespace DXMPP
{
    namespace SASL
    {
        namespace Weak
        {
            class SASL_Mechanism_DigestMD5 : public SASLMechanism
            {
                std::string GetMD5Hex(std::string Input);
                std::string GetHA1(std::string X, std::string nonce, std::string cnonce);
            public:
                void Begin();

                SASL_Mechanism_DigestMD5(std::shared_ptr<DXMPP::Network::AsyncTCPXMLClient> Uplink,
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
}


#endif // DXMPP_SASLMechanism_DIGESTMD5_hpp
