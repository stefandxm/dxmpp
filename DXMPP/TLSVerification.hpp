//
//  TLSVerification.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_TLSVerification_hpp
#define DXMPP_TLSVerification_hpp

#include <boost/asio/ssl/verify_context.hpp>

namespace DXMPP
{
    enum class TLSVerificationMode
    {
        RFC2818_Hostname,
        Custom,
        None
    };

    class TLSVerification
    {
    public:
        TLSVerificationMode Mode;

        virtual bool VerifyCertificate(bool Preverified,
                                       boost::asio::ssl::verify_context& ctx)
        {
            return false;
        }

        TLSVerification()
            :
              Mode(TLSVerificationMode::RFC2818_Hostname)
        {
        }
        TLSVerification(TLSVerificationMode Mode)
            :
              Mode(Mode)
        {
        }
    };
}

#endif // DXMPP_TLSVerification_hpp
