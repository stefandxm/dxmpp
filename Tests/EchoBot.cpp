//
//  EchoBot.cpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/Connection.hpp>
#include <boost/thread.hpp>

using namespace std;
using namespace DXMPP;
using namespace pugi;

class EchoBot :
        public StanzaCallback,
        public ConnectionCallback
{
public:
    volatile bool Quit;

    EchoBot()
        :
          Quit(false)
    {
    }

    void StanzaReceived(SharedStanza Stanza,
            SharedConnection Sender)
    {
        xml_node Body = Stanza->Message.select_single_node("//body").node();
        if(!Body)
            return;

        if( string(Body.text().as_string()) == "quit")
            Quit = true;

        cout << "Echoing message '" << Body.text().as_string() << "' from " << Stanza->From.GetFullJID() << std::endl;


        SharedStanza ResponseStanza = Sender->CreateStanza(Stanza->From);
        ResponseStanza->Message.append_copy(Body);
        Sender->SendStanza(ResponseStanza);
    }

    void ConnectionStateChanged(ConnectionState NewState,
        SharedConnection /*Sender*/)
    {
        switch(NewState)
        {
            case ConnectionState::Connected:
                cout << "\\o/ Lets annoy!" << endl;
                break;
            case ConnectionState::Connecting:
                cout << "-o/ Lets go allready.. Connect connect connect!!" << endl;
                break;
            case ConnectionState::ErrorAuthenticating:
            case ConnectionState::ErrorConnecting:
            case ConnectionState::ErrorUnknown:
                cerr << "-o/~ Mercy mercy i am bugged!!" << endl;
                Quit = true;
                break;
            case ConnectionState::NotConnected:
                cout << "-o- WHY NOT CONNECTED?!" << endl;
                break;
        }
    }
};

int main(int, const char **)
{
    EchoBot Handler;

    /*SharedConnection Uplink =*/ Connection::Create( string("deusexmachinae.se") /* Host */,
                                                  5222 /* Port number */,
                                                  DXMPP::JID( "dxmpp@users/test" ) /* Requested JID */,
                                                  string("dxmpp") /* Password */,
                                                  &Handler /* Stanza callback handler */,
                                                  &Handler /* Connection callback handler */);

    std::cout << "Entering fg loop." <<std::endl;
    while(!Handler.Quit)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }   
}
