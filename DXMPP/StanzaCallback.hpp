//
//  StanzaCallback.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_StanzaCallBack_hpp
#define DXMPP_StanzaCallBack_hpp

#include "pugixml/pugixml.hpp"
#include "JID.hpp"
#include <string>
#include "Stanza.hpp"
#include <memory>


namespace  DXMPP
{
    class Connection;
    typedef std::shared_ptr<Connection> SharedConnection;

    class StanzaCallback
    {
    public:
        virtual void StanzaReceived(SharedStanza Stanza,
            SharedConnection Sender) = 0;

        virtual ~StanzaCallback()
        {
        }
    };
}

#endif // DXMPP_StanzaCallBack_hpp
