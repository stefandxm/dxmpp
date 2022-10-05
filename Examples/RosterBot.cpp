//
//  RosterBot.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/Connection.hpp>
#include <boost/thread.hpp>

using namespace std;
using namespace DXMPP;
using namespace pugi;

class RosterBot :
        public IEventHandler,
        public StanzaCallback,
        public ConnectionCallback,
        public PresenceCallback,
        public SubscribeCallback,
        public SubscribedCallback,
        public UnsubscribedCallback
{
public:
    volatile bool Quit;

    RosterBot()
        :
          Quit(false)
    {
    }

    void StanzaReceived(SharedStanza Stanza,
            SharedConnection Sender)
    {
        xml_node Body = Stanza->Payload.select_node("//body").node();
        if(!Body)
            return;

        if( string(Body.text().as_string()) == "quit")
        {
            cout << "Received quit. Quiting" << endl;
            Quit = true;
            return;
        }

        cout << "Echoing message '" << Body.text().as_string() << "' from "
             << Stanza->From.GetFullJID() << endl;


        SharedStanza ResponseStanza = Sender->CreateStanza(Stanza->From, StanzaType::Message);
        ResponseStanza->Payload.append_copy(Body);
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

    void OnPresence(JID From,
                    bool Available,
                    int Priority,
                    string Status,
                    string Message)
    {
        cout << "Got presence from " << From.GetFullJID()
             << " available: " << Available
             << " priority " << Priority << " status \""
             << Status << "\" with message \""
             << Message << "\"" << endl;
    }

    SubscribeCallback::Response OnSubscribe(JID From)
    {
        cout << "Got subscribe request from " << From.GetFullJID()
             << " I've decided to make friend with her" << endl;

        return SubscribeCallback::Response::AllowAndSubscribe;
    }

    void OnSubscribed(JID To)
    {
        cout << "I am now friend with " << To.GetFullJID() << endl;
    }

    void OnUnsubscribed(JID From)
    {
        cout << From.GetFullJID() << " doesnt want to be friend with me anymore."
             << " I will cancel my friendship with her!" << endl;
    }
};

int main(int, const char **)
{
    RosterBot Handler;

    // Please note we are using selfsigned certificates on dev server so need to pass
    // TLSVerificationMode::None. This should not be done in production.
    SharedConnection Uplink = Connection::Create( string("deusexmachinae.se") /* Host */,
                                                  5222 /* Port number */,
                                                  DXMPP::JID( "dxmpp@users" ) /* Requested JID */,
                                                  string("dxmpp") /* Password */,
                                                  &Handler,
                                                  TLSVerificationMode::None
                                                  );

    cout << "Entering fg loop." << endl;
    while(!Handler.Quit)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }   

    return 0;
}
