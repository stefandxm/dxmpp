//
//  Roster.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_Roster_hpp
#define DXMPP_Roster_hpp

#include <DXMPP/pugixml/pugixml.hpp>
#include <DXMPP/Network/AsyncTCPXMLClient.hpp>
#include <DXMPP/JID.hpp>

namespace  DXMPP
{
    class PresenceCallback
    {
    public:
        virtual void OnPresence(JID From, bool Available, int Priority, std::string Status, std::string Message) = 0;
    };

    class SubscribeCallback
    {
    public:
        enum class Response
        {
            Allow,
            AllowAndSubscribe,
            Reject
        };

        virtual Response OnSubscribe(JID From) = 0;
    };

    class SubscribedCallback
    {
    public:
        virtual void OnSubscribed(JID To) = 0;
    };

    class UnsubscribedCallback
    {
    public:
        virtual void OnUnsubscribed(JID From) = 0;
    };


    class RosterMaintaner
    {
        friend class Connection;

        DXMPP::Network::AsyncTCPXMLClient *Uplink;

        PresenceCallback *PresenceHandler;
        SubscribeCallback *SubscribeHandler;
        SubscribedCallback *SubscribedHandler;
        UnsubscribedCallback *UnsubscribedHandler;


        void HandleSubscribe(pugi::xml_node Node);
        void HandleSubscribed(pugi::xml_node Node);
        void HandleError(pugi::xml_node Node);
        void HandleUnsubscribe(pugi::xml_node Node);
        void HandleUnsubscribed(pugi::xml_node Node);

        // Invoked from Connection
        void OnPresence(pugi::xml_node Node);
    public:


        // User functions
        void Subscribe(JID To);
        void Unsubscribe(JID To);

        RosterMaintaner(DXMPP::Network::AsyncTCPXMLClient *Uplink,
                        PresenceCallback *PresenceHandler,
                        SubscribeCallback *SubscribeHandler,
                        SubscribedCallback *SubscribedHandler,
                        UnsubscribedCallback *UnsubscribedHandler)
            :
              Uplink(Uplink),
              PresenceHandler(PresenceHandler),
              SubscribeHandler(SubscribeHandler),
              SubscribedHandler(SubscribedHandler),
              UnsubscribedHandler(UnsubscribedHandler)
        {
        }

    };
}

#endif // DXMPP_Roster_hpp
