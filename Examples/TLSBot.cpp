//
//  TLSBot.cpp
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

class TLSBot :
        public StanzaCallback,
        public ConnectionCallback
{
public:
    volatile bool Quit;

    TLSBot()
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
        {
            cout << "Received quit. Quiting" << endl;
            Quit = true;
            return;
        }

        cout << "Echoing message '" << Body.text().as_string() << "' from "
             << Stanza->From.GetFullJID() << endl;


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

class CustomTLSVerification
        : public TLSVerification
{
public:
    virtual bool VerifyCertificate(bool /*Preverified*/,
                                   boost::asio::ssl::verify_context& /*ctx*/)
    {
        cout << "I should seriously verify this, but for now i will just reject" << endl;
        return false;
    }


    CustomTLSVerification()
        :
          TLSVerification(TLSVerificationMode::Custom)
    {
    }

};


int main(int, const char **)
{
    TLSBot Handler;

    CustomTLSVerification Verifier;

    SharedConnection Uplink = Connection::Create( string("deusexmachinae.se") /* Host */,
                                                  5222 /* Port number */,
                                                  DXMPP::JID( "dxmpp@users" ) /* Requested JID */,
                                                  string("dxmpp") /* Password */,
                                                  &Handler /* Connection callback handler */,
                                                  &Handler /* Stanza callback handler */,
                                                  nullptr,nullptr,nullptr,nullptr,&Verifier);

    cout << "Entering fg loop." <<endl;
    while(!Handler.Quit)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }   

    return 0;
}
