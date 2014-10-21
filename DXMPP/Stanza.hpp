//
//  JID.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
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
        pugi::xml_document *Document;
        pugi::xml_node Message;

        JID To;
        JID From;

        StanzaType Type;

        std::string ID;

        Stanza(pugi::xml_document *Document, pugi::xml_node Message)
            :Document(Document), Message(Message)
        {
            To = JID(Message.attribute("from").value());
            From = JID(Message.attribute("from").value());

            if( std::string(Message.attribute("type").value()) == "error")
                Type = StanzaType::Error;
            if( std::string(Message.attribute("type").value()) == "chat")
                Type = StanzaType::Chat;
        }

        Stanza()
            :Type(StanzaType::Chat)
        {
            Document = new pugi::xml_document();
            
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
            delete Document;
        }

    };

    typedef boost::shared_ptr<Stanza> SharedStanza;

}

#endif
