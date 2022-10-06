//
//  Stanza.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_Stanza_hpp
#define DXMPP_Stanza_hpp

#include "pugixml/pugixml.hpp"
#include "JID.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>

namespace DXMPP {
    enum class StanzaType
    {
        IQ,
        Message,
        Presence
    };

    enum class StanzaMessageType
    {
        Chat,
        Error,
        Groupchat,
        Headline,
        Normal
    };

    enum class StanzaIQType
    {
        Get,
        Set,
        Result,
        Error
    };

    class Stanza
    {

    public:
        std::unique_ptr<pugi::xml_document> Document;
        pugi::xml_node Payload;

        JID To;
        JID From;

        StanzaType Type;

        std::string ID;

        Stanza(StanzaType Type, std::unique_ptr<pugi::xml_document> Document, pugi::xml_node Payload)
            : Document( std::move(Document) ), Payload(Payload), Type(Type)
        {
            ID = std::string(Payload.attribute("id").value());
            To = JID(Payload.attribute("to").value());
            From = JID(Payload.attribute("from").value());
        }

        Stanza(StanzaType Type)
            :Type(Type)
        {
        }

        virtual void ProvisionOutgoingStanza() = 0;

        virtual ~Stanza()
        {
        }

    };

    class StanzaMessage : public Stanza
    {
    public:
        StanzaMessageType MessageType;

        void ProvisionOutgoingStanza()
        {
            switch(MessageType)
            {
            case StanzaMessageType::Chat:
                Payload.attribute("type").set_value( "chat" );
                break;
            case StanzaMessageType::Normal:
                Payload.attribute("type").set_value( "normal" );
                break;
            case StanzaMessageType::Groupchat:
                Payload.attribute("type").set_value( "groupchat" );
                break;
            case StanzaMessageType::Headline:
                Payload.attribute("type").set_value( "headline" );
                break;
            case StanzaMessageType::Error:
                Payload.attribute("type").set_value( "error" );
                break;
            }
        }

        StanzaMessage(std::unique_ptr<pugi::xml_document> Document, pugi::xml_node Payload)
            : Stanza( StanzaType::Message, std::move(Document), Payload)
        {
            MessageType = StanzaMessageType::Normal;

            if( std::string(Payload.attribute("type").value()) == "normal")
                MessageType = StanzaMessageType::Normal;
            if( std::string(Payload.attribute("type").value()) == "headline")
                MessageType = StanzaMessageType::Headline;
            if( std::string(Payload.attribute("type").value()) == "groupchat")
                MessageType = StanzaMessageType::Groupchat;
            if( std::string(Payload.attribute("type").value()) == "error")
                MessageType = StanzaMessageType::Error;
            if( std::string(Payload.attribute("type").value()) == "chat")
                MessageType = StanzaMessageType::Chat;
        }


        StanzaMessage()
            : Stanza(StanzaType::Message), MessageType(StanzaMessageType::Chat)
        {
            Document.reset(new pugi::xml_document());

            Payload = Document->append_child("message");
            Payload.append_attribute("from");
            Payload.append_attribute("to");
            Payload.append_attribute("type");
            Payload.append_attribute("id");

            if(ID.length() == 0)
            {
                ID = boost::lexical_cast<std::string>( boost::uuids::random_generator()() );
            }

            Payload.attribute("id").set_value( ID.c_str() );
        }

        ~StanzaMessage()
        {
        }
    };


    class StanzaIQ: public Stanza
    {
    public:
        StanzaIQType IQType;

        void ProvisionOutgoingStanza()
        {
            switch(IQType)
            {
            case StanzaIQType::Get:
                Payload.attribute("type").set_value( "get" );
                break;
            case StanzaIQType::Set:
                Payload.attribute("type").set_value( "set" );
                break;
            case StanzaIQType::Result:
                Payload.attribute("type").set_value( "result" );
                break;
            case StanzaIQType::Error:
                Payload.attribute("type").set_value( "error" );
                break;
            }
        }

        StanzaIQ(std::unique_ptr<pugi::xml_document> Document, pugi::xml_node Payload)
            : Stanza(StanzaType::IQ,std::move(Document), Payload)
        {

            std::string strtype = std::string(Payload.attribute("type").value());

            if( strtype == "error")
                IQType = StanzaIQType::Error;
            if( strtype == "get")
                IQType = StanzaIQType::Get;
            if( strtype == "set")
                IQType = StanzaIQType::Set;
            if( strtype == "result")
                IQType = StanzaIQType::Result;
        }


        StanzaIQ()
            : Stanza(StanzaType::Message), IQType(StanzaIQType::Get)
        {
            Document.reset(new pugi::xml_document());

            Payload = Document->append_child("iq");
            Payload.append_attribute("from");
            Payload.append_attribute("to");
            Payload.append_attribute("type");
            Payload.append_attribute("id");

            if(ID.length() == 0)
            {
                ID = boost::lexical_cast<std::string>( boost::uuids::random_generator()() );
            }

            Payload.attribute("id").set_value( ID.c_str() );
        }

        ~StanzaIQ()
        {
        }
    };


    typedef std::shared_ptr<Stanza> SharedStanza;
    typedef std::shared_ptr<StanzaMessage> SharedStanzaMessage;
    typedef std::shared_ptr<StanzaIQ> SharedStanzaIQ;

}

#endif
