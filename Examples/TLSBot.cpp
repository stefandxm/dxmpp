//
//  TLSBot.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#include <DXMPP/Connection.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>


using namespace std;
using namespace DXMPP;
using namespace pugi;

class TLSBot :
        public IEventHandler,
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
};

class CustomTLSVerification
        : public TLSVerification
{
public:
    bool VerifyCertificate(bool /*Preverified*/,
                           boost::asio::ssl::verify_context& /*ctx*/)
    {
        cout << "I should seriously verify this, but for now i will just reject" << endl;
        return true;
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

    boost::asio::const_buffer Certificate;
    boost::asio::const_buffer PrivateKey;
    std::string Hostname = "host";
    std::string Domain = "domain";


    {
        ifstream certfile ("/home/stefan/certs/cpptest.pem", std::ios::binary | std::ios::ate);
        std::streamsize certsize = certfile.tellg();
        certfile.seekg(0, std::ios::beg);
        std::vector<char> buffecertr(certsize);
        if (!certfile.read(buffecertr.data(), certsize))
        {
            std::cerr << "Couldn't find certfile" << std::endl;
            return 1;
        }
        Certificate = boost::asio::const_buffer(buffecertr.data(), buffecertr.size());
    }


    {
        ifstream pkeyfile ("/home/stefan/certs/cpptest.pem", std::ios::binary | std::ios::ate);
        std::streamsize pkeysize = pkeyfile.tellg();
        pkeyfile.seekg(0, std::ios::beg);
        std::vector<char> bufferkey(pkeysize);
        if (!pkeyfile.read(bufferkey.data(), pkeysize))
        {
            std::cerr << "Couldn't find keyfile" << std::endl;
            return 1;
        }
        PrivateKey = boost::asio::const_buffer(bufferkey.data(), bufferkey.size());
     }

    SharedConnection Uplink = Connection::Create( Hostname,
                                                  5222 /* Port number */,
                                                  Domain,
                                                  Certificate,
                                                  PrivateKey,
                                                  &Handler,
                                                  &Verifier,
                                                  DebugOutputTreshold::Debug);


    cout << "Entering fg loop." <<endl;
    while(!Handler.Quit)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }   

    return 0;
}
