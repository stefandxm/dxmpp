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
#include <boost/shared_ptr.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>

namespace DXMPP {
    enum class StanzaType
    {
        Chat,
        Error
    };

    class Stanza
    {

    public:
        std::unique_ptr<pugi::xml_document> Document;
        pugi::xml_node Message;

        JID To;
        JID From;

        StanzaType Type;

        std::string ID;

        Stanza(std::unique_ptr<pugi::xml_document> Document, pugi::xml_node Message)
            :Document( std::move(Document) ), Message(Message)
        {
            To = JID(Message.attribute("to").value());
            From = JID(Message.attribute("from").value());

            if( std::string(Message.attribute("type").value()) == "error")
                Type = StanzaType::Error;
            if( std::string(Message.attribute("type").value()) == "chat")
                Type = StanzaType::Chat;
        }

        Stanza()
            :Type(StanzaType::Chat)
        {
            Document.reset(new pugi::xml_document());

            Message = Document->append_child("message");
            Message.append_attribute("from");
            Message.append_attribute("to");
            Message.append_attribute("type");
            Message.append_attribute("id");

            if(ID.length() == 0)
            {
                ID = boost::lexical_cast<std::string>( boost::uuids::random_generator()() );
            }

            Message.attribute("id").set_value( ID.c_str() );
        }

        ~Stanza()
        {
        }

    };

    typedef boost::shared_ptr<Stanza> SharedStanza;

}

#endif
