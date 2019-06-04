//
//  SASLMechanism_PLAIN.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2019
//  Copyright (c) 2019 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_SASLMechanism_EXTERNAL_hpp
#define DXMPP_SASLMechanism_EXTERNAL_hpp

#include "SASLMechanism.hpp"

namespace DXMPP
{
    namespace SASL
    {
        class SASL_Mechanism_EXTERNAL: public SASLMechanism
        {
        public:
            void Begin();

            SASL_Mechanism_EXTERNAL(boost::shared_ptr<DXMPP::Network::AsyncTCPXMLClient> Uplink)
                :SASLMechanism(Uplink, JID(""), std::string(""))
            {
            }

            void Challenge(const pugi::xpath_node &challenge);
            bool Verify(const pugi::xpath_node &SuccessTag);
        };
    }
}


#endif // DXMPP_SASLMechanism_EXTERNAL_hpp
