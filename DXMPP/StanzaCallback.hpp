#ifndef DXMPP_StanzaCallBack_hpp
#define DXMPP_StanzaCallBack_hpp

#include "pugixml/pugixml.hpp"
#include "JID.hpp"
#include <string>
#include <boost/shared_ptr.hpp>
#include "Stanza.hpp"
#include <memory>

namespace  DXMPP
{
    class Connection;
    typedef boost::shared_ptr<Connection> SharedConnection;

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
