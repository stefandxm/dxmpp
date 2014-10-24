//
//  Roster.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/Roster.hpp>
#include <boost/lexical_cast.hpp>

namespace DXMPP
{
    void RosterMaintaner::HandleSubscribe(pugi::xml_node Node)
    {
        if(SubscribeHandler == nullptr)
            return;

        JID From(Node.attribute("from").value());

        SubscribeHandler->OnSubscribe(From);
    }

    void RosterMaintaner::HandleSubscribed(pugi::xml_node Node)
    {
        if(SubscribedHandler == nullptr)
            return;

        JID From(Node.attribute("from").value());

        SubscribedHandler->OnSubscribed(From);
    }

    void RosterMaintaner::HandleError(pugi::xml_node Node)
    {
        JID From(Node.attribute("from").value());
        std::cerr << "Received error presence from " << From.GetFullJID() << std::endl;
    }

    void RosterMaintaner::HandleUnsubscribed(pugi::xml_node Node)
    {
        if(UnsubscribedHandler == nullptr)
            return;

        JID From(Node.attribute("from").value());

        UnsubscribedHandler->OnUnsubscribed(From);
    }

    void RosterMaintaner::OnPresence(pugi::xml_node Node)
    {
        std::string Type = Node.attribute("type").value();

        if(Type == "subscribe")
        {
            HandleSubscribe(Node);
            return;
        }
        if(Type == "subscribed")
        {
            HandleSubscribe(Node);
            return;
        }
        if(Type == "unsubscribe")
        {
            HandleSubscribe(Node);
            return;
        }
        if(Type == "unsubscribed")
        {
            HandleSubscribed(Node);
            return;
        }

        if(PresenceHandler == nullptr)
            return;

        JID From(Node.attribute("from").value());
        int Priority = 0;

        Priority = Node.child("priority").text().as_int();

        pugi::xml_node ShowTag = Node.child("show");
        std::string Show = ShowTag.text().as_string();

        pugi::xml_node StatusTag = Node.child("status");
        std::string Status = StatusTag.text().as_string();

        PresenceHandler->OnPresence(From, Type != "unavailable", Priority, Status, Show);
    }

    void RosterMaintaner::Subscribe(JID To, std::string Message)
    {
        pugi::xml_document doc;
        pugi::xml_node PresenceTag = doc.append_child("presence");
        pugi::xml_node SubscribeTag = PresenceTag.append_child("subscribe");

        SubscribeTag.append_attribute("to");
        SubscribeTag.attribute("to").set_value(To.GetBareJID().c_str());

        Uplink->WriteXMLToSocket(&doc);
    }

    void RosterMaintaner::Unsubscribe(JID To)
    {
        pugi::xml_document doc;
        pugi::xml_node PresenceTag = doc.append_child("presence");
        pugi::xml_node SubscribeTag = PresenceTag.append_child("unsubscribe");

        SubscribeTag.append_attribute("to");
        SubscribeTag.attribute("to").set_value(To.GetBareJID().c_str());

        Uplink->WriteXMLToSocket(&doc);
    }


}
