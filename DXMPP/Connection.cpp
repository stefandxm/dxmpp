//
//  Connection.cpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#include "Connection.hpp"


#include <boost/thread.hpp>

#include <iostream>

#include <DXMPP/SASL/SASLMechanism.hpp>
#include <DXMPP/SASL/SASLMechanism_DIGEST_MD5.hpp>
#include <DXMPP/SASL/SASLMechanism_SCRAM_SHA1.hpp>
#include <DXMPP/SASL/SASLMechanism_PLAIN.hpp>

namespace DXMPP
{

#define DebugOut(DebugLevel) \
if (DebugLevel > DebugTreshold) {} \
else std::cout

    using namespace std;
    using namespace pugi;
    
    
    void Connection::BrodcastConnectionState(ConnectionCallback::ConnectionState NewState)
    {
        if(PreviouslyBroadcastedState == NewState)
            return;
        PreviouslyBroadcastedState = NewState;

        if(!ConnectionHandler)
            return;

        ConnectionHandler->ConnectionStateChanged(NewState, shared_from_this());
    }


    void Connection::OpenXMPPStream()
    {
        CurrentConnectionState = ConnectionState::WaitingForFeatures;

        stringstream Stream;
        Stream << "<?xml version='1.0' encoding='utf-8'?>" << std::endl;
        Stream << "<stream:stream" << endl;
        Stream << " from = '" << MyJID.GetBareJID() << "'" << endl;
        Stream << " to = '" << MyJID.GetDomain() << "'" << endl;
        Stream << " version='1.0'" << endl;
        Stream << " xml:lang='en'" << endl;
        Stream << " xmlns='jabber:client'" << endl;
        Stream << " xmlns:stream='http://etherx.jabber.org/streams'>";

        DebugOut(DebugOutputTreshold::Debug) << "DXMPP: Opening stream" << std::endl;// << Stream.str();

        Client.WriteTextToSocket(Stream.str());
    }

    void Connection::CheckStreamForFeatures()
    {
        string str = Client.ReadDataStream->str();
        size_t streamfeatures = str.find("</stream:features>");
            
        if(streamfeatures == string::npos)
            return;
            
        if(CurrentAuthenticationState == AuthenticationState::SASL)
        {
            Client.ClearReadDataStream();
            CurrentConnectionState = ConnectionState::Authenticating;
            return;
        }
        if(CurrentAuthenticationState == AuthenticationState::Bind)
        {
            Client.ClearReadDataStream();
            CurrentConnectionState = ConnectionState::Authenticating;
            BindResource();
            return;
        }
        
        // note to self: cant use loadxml() here because this is not valid xml!!!!
        // due to <stream> <stream::features></stream::features>
        xml_document xdoc;
        xdoc.load(*Client.ReadDataStream, parse_full&~parse_eol, encoding_auto);
            
        Client.ClearReadDataStream();

        ostringstream o;
            
        xdoc.save(o, "\t", format_no_declaration);

        pugi::xpath_node starttls = xdoc.select_single_node("//starttls");
        if(starttls)
        {
            DebugOut(DebugOutputTreshold::Debug)  << std::endl << "START TLS SUPPORTED" << std::endl;
            FeaturesStartTLS = true;
        }

        //Move to sasl class
        pugi::xpath_node_set mechanisms = xdoc.select_nodes("//mechanism");
        for (auto it = mechanisms.begin(); it != mechanisms.end(); it++)
        {
            xml_node node = it->node();
            string mechanism = string(node.child_value());
            DebugOut(DebugOutputTreshold::Debug) << "Mechanism supported: " << mechanism << std::endl;
                

            if(mechanism == "DIGEST-MD5")
                FeaturesSASL_DigestMD5 = true;
            if(mechanism == "CRAM-MD5")
                FeaturesSASL_CramMD5 = true;
            if(mechanism == "SCRAM-SHA-1")
                FeaturesSASL_ScramSHA1 = true;
            if(mechanism == "PLAIN")
                FeaturesSASL_Plain = true;
                
        }
            
        CurrentConnectionState = ConnectionState::Authenticating;
            
        // If start tls: initiate shit/restart stream
            
        if(CurrentAuthenticationState != AuthenticationState::StartTLS)
        {
            if(FeaturesStartTLS)
            {
                CurrentAuthenticationState = AuthenticationState::StartTLS;
                    
                DebugOut(DebugOutputTreshold::Debug) << "Initializing TLS" << std::endl;
                    
                stringstream Stream;
                Stream << "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>";
                Client.WriteTextToSocket(Stream.str());
                return;
            }
        }
        CurrentAuthenticationState = AuthenticationState::SASL;
            
            
        DebugOut(DebugOutputTreshold::Debug) << "SASL MANDLEBRASL" << std::endl;
        // I shall has picked an algorithm!
        
        if(FeaturesSASL_ScramSHA1)
        {
            Authentication = new  SASL::SASL_Mechanism_SCRAM_SHA1 ( &Client, MyJID, Password),
            Authentication->Begin();
            return;
        }
        if(FeaturesSASL_DigestMD5)
        {
            Authentication = new  SASL::Weak::SASL_Mechanism_DigestMD5 ( &Client , MyJID, Password),
            Authentication->Begin();
            return;
        }
        if(FeaturesSASL_Plain)
        {
            Authentication = new  SASL::Weak::SASL_Mechanism_PLAIN ( &Client , MyJID, Password),
            Authentication->Begin();
            return;
        }
    }


    void Connection::InitTLS()
    {
        DebugOut(DebugOutputTreshold::Debug) << "Server accepted to start TLS handshake" << std::endl;
        bool Success = Client.ConnectTLSSocket();
        if(Success)
        {
            DebugOut(DebugOutputTreshold::Debug) << "TLS Connection successfull. Reopening stream." << std::endl;
            OpenXMPPStream();
        }
        else
            std::cerr << "TLS Connection failed" << std::endl;

    }
        
    // Explicit string hax
    void Connection::CheckForStreamEnd()
    {
        string str = Client.ReadDataStream->str();
        size_t streamend = str.find("</stream:stream>");
        if(streamend == string::npos)
            streamend = str.find("</stream>");
            
        if(streamend == string::npos)
            return;
            
        Client.ClearReadDataStream();

        CurrentConnectionState = ConnectionState::ErrorUnknown;

        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            
        DebugOut(DebugOutputTreshold::Debug) << "Got stream end" << std::endl;
    }

    void Connection::CheckForTLSProceed()
    {
        if(!Client.LoadXML())
            return;

        if(!Client.SelectSingleXMLNode("//proceed"))
        {
            std::cerr << "No proceed tag; B0rked SSL?!";

            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;
            return;
        }

        if(CurrentAuthenticationState == AuthenticationState::StartTLS)
            InitTLS();
    }
        
    void Connection::CheckForWaitingForSession()
    {
        if(!Client.LoadXML())
            return;

        xml_node iqnode = Client.SelectSingleXMLNode("//iq");

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }

        // TODO: Verify iq response..

        string Presence = "<presence/>";

        Client.WriteTextToSocket(Presence);
        CurrentConnectionState = ConnectionState::Connected;
        DebugOut(DebugOutputTreshold::Debug) << std::endl << "ONLINE" << std::endl;
    }

    void Connection::CheckForBindSuccess()
    {
        if(!Client.LoadXML())
            return;

        xml_node iqnode = Client.SelectSingleXMLNode("//iq");

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }
            
        DebugOut(DebugOutputTreshold::Debug) << std::endl<< "AUTHENTICATED" << std::endl; // todo: verify xml ;)
            
        string StartSession = "<iq type='set' id='1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
        Client.WriteTextToSocket(StartSession);
        CurrentConnectionState = ConnectionState::WaitingForSession;
        CurrentAuthenticationState = AuthenticationState::Authenticated;
    }
        
    void Connection::BindResource()
    {
        // TODO: Make Proper XML ?
        //bind resource..
        stringstream TStream;
        TStream << "<iq type='set' id='bindresource'>";
        TStream << "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>";
        TStream << "<resource>" << MyJID.GetResource() << "</resource>";
        TStream << "</bind>";
        TStream << "</iq>";
        Client.WriteTextToSocket(TStream.str());
    }

    void Connection::StartBind()
    {
        CurrentAuthenticationState = AuthenticationState::Bind;
        OpenXMPPStream();
    }

    void Connection::CheckForSASLData()
    {
        if(!Client.LoadXML())
            return;

        xml_node challenge = Client.SelectSingleXMLNode("//challenge");
        xml_node success = Client.SelectSingleXMLNode("//success");

        if(!challenge && !success)
        {           
            std::cerr << "Bad authentication." << std::endl;
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorAuthenticating);
            CurrentConnectionState = ConnectionState::ErrorAuthenticating;

            return;
        }

        if(challenge)
        {
            Authentication->Challenge(challenge);
            return;
        }

        if(success)
        {
            if( !Authentication->Verify(success) )
            {
                std::cerr << "Bad success verification from server" << std::endl;
                BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorAuthenticating);
                CurrentConnectionState = ConnectionState::ErrorAuthenticating;

                return;
            }
            DebugOut(DebugOutputTreshold::Debug) << std::endl<< "Authentication succesfull." << std::endl;
            StartBind();
        }
    }

    void Connection::CheckStreamForAuthenticationData()
    {
        switch(CurrentAuthenticationState)
        {
            case AuthenticationState::StartTLS:
                CheckForTLSProceed();
                break;
            case AuthenticationState::SASL:
                CheckForSASLData();
                break;
            case AuthenticationState::Bind:
                CheckForBindSuccess();
                break;
            case AuthenticationState::Authenticated:
                break;
        default:
            break;
        }
        
    }
        
    void Connection::CheckStreamForStanza()
    {
        if(!Client.LoadXML())
            return;

        xpath_node message = Client.SelectSingleXMLNode("//message");
        if(!message)
            return;

        if(StanzaHandler)
            StanzaHandler->StanzaReceived( SharedStanza(new Stanza(Client.FetchDocument(), message.node())), shared_from_this());
    }

    void Connection::CheckForPresence()
    {
        if(!Client.LoadXML())
            return;

        xpath_node presence = Client.SelectSingleXMLNode("//presence");
        if(!presence)
            return;

        Roster.OnPresence(presence.node());
    }


    SharedStanza Connection::CreateStanza(const JID &TargetJID)
    {
        SharedStanza ReturnValue(new Stanza());
        ReturnValue->To = TargetJID;
        ReturnValue->From = MyJID;
        return ReturnValue;
    }

    void Connection::SendStanza(SharedStanza Stanza)
    {
        switch(Stanza->Type)
        {
        case StanzaType::Chat:
            Stanza->Message.attribute("type").set_value( "chat" );
            break;
        case StanzaType::Error:
            Stanza->Message.attribute("type").set_value( "error" );
            break;
        }

        Stanza->Message.attribute("from").set_value( MyJID.GetFullJID().c_str() );
        Stanza->Message.attribute("to").set_value( Stanza->To.GetFullJID().c_str() );

        Client.WriteXMLToSocket(Stanza->Document);
    }   

    void Connection::CheckStreamForValidXML()
    {
        switch(CurrentConnectionState)
        {
            case ConnectionState::WaitingForSession:
                BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
                CheckForWaitingForSession();
                break;
            case ConnectionState::WaitingForFeatures:
                BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
                CheckStreamForFeatures();
                break;
            case ConnectionState::Authenticating:
                BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
                CheckStreamForAuthenticationData();
                break;
            case ConnectionState::Connected:
                BrodcastConnectionState(ConnectionCallback::ConnectionState::Connected);
                CheckForPresence();
                CheckStreamForStanza();
                break;
        default:
            break;
        }
            
        CheckForStreamEnd();
    }
        

    Connection::Connection(const std::string &Hostname,
        int Portnumber,
        const JID &RequestedJID,
        const std::string &Password,
        ConnectionCallback *ConnectionHandler,
        StanzaCallback *StanzaHandler,
        PresenceCallback *PresenceHandler,
        SubscribeCallback *SubscribeHandler,
        SubscribedCallback *SubscribedHandler,
        UnsubscribedCallback *UnsubscribedHandler,
        TLSVerification *Verification,
        TLSVerificationMode VerificationMode,
        DebugOutputTreshold DebugTreshold)
    :
        SelfHostedVerifier(new TLSVerification(VerificationMode)),
        ConnectionHandler(ConnectionHandler),
        StanzaHandler(StanzaHandler),
        Client(
                ((Verification != nullptr) ? Verification : SelfHostedVerifier.get()),
                Hostname,
                Portnumber,
                boost::bind(&Connection::ClientDisconnected, this),
                boost::bind(&Connection::ClientGotData, this),
                DebugTreshold ),
        DebugTreshold(DebugTreshold),
        CurrentAuthenticationState(AuthenticationState::None),
        MyJID(RequestedJID),
        Roster(&Client,
               PresenceHandler,
               SubscribeHandler,
               SubscribedHandler,
               UnsubscribedHandler)
    {
        FeaturesStartTLS = false;

        this->Password = Password;
        Connect();
    }

    void Connection::Connect()
    {
        DebugOut(DebugOutputTreshold::Debug) << "Starting io_service run in background thread" << std::endl;

        PreviouslyBroadcastedState = ConnectionCallback::ConnectionState::Connecting;
        if( !Client.ConnectSocket() )
        {
            CurrentConnectionState = ConnectionState::ErrorConnecting;
            std::cerr << "DXMPP: Failed to connect" << std::endl;
            return;
        }
        OpenXMPPStream();
        Client.AsyncRead();
        Client.ForkIO();
    }

    Connection::~Connection()
    {
        DebugOut(DebugOutputTreshold::Debug) << "~Connection"  << std::endl;
    }



    SharedConnection Connection::Create(const std::string &Hostname,
                                    int Portnumber,
                                    const JID &RequestedJID,
                                    const std::string &Password,
                                    TLSVerificationMode VerificationMode,
                                    ConnectionCallback *ConnectionHandler,
                                    StanzaCallback *StanzaHandler,
                                    PresenceCallback *PresenceHandler,
                                    SubscribeCallback *SubscribeHandler,
                                    SubscribedCallback *SubscribedHandler,
                                    UnsubscribedCallback *UnsubscribedHandler,
                                    DebugOutputTreshold DebugTreshold)
    {
        return boost::shared_ptr<Connection>(
                    new Connection(Hostname,
                                   Portnumber,
                                   RequestedJID,
                                   Password,
                                   ConnectionHandler,
                                   StanzaHandler,
                                   PresenceHandler,
                                   SubscribeHandler,
                                   SubscribedHandler,
                                   UnsubscribedHandler,
                                   nullptr,
                                   VerificationMode,
                                   DebugTreshold)
                );
    }


    SharedConnection Connection::Create(const std::string &Hostname,
                                    int Portnumber,
                                    const JID &RequestedJID,
                                    const std::string &Password,
                                    ConnectionCallback *ConnectionHandler,
                                    StanzaCallback *StanzaHandler,
                                    PresenceCallback *PresenceHandler,
                                    SubscribeCallback *SubscribeHandler,
                                    SubscribedCallback *SubscribedHandler,
                                    UnsubscribedCallback *UnsubscribedHandler,
                                    TLSVerification *Verification,
                                    DebugOutputTreshold DebugTreshold)
    {
        return boost::shared_ptr<Connection>(
                    new Connection(Hostname,
                                   Portnumber,
                                   RequestedJID,
                                   Password,
                                   ConnectionHandler,
                                   StanzaHandler,
                                   PresenceHandler,
                                   SubscribeHandler,
                                   SubscribedHandler,
                                   UnsubscribedHandler,
                                   Verification,
                                   (Verification!=nullptr?Verification->Mode:TLSVerificationMode::RFC2818_Hostname),
                                   DebugTreshold)
                );
    }



    void Connection::ClientDisconnected()
    {
        CurrentConnectionState  = ConnectionState::ErrorUnknown;
        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
    }
    void Connection::ClientGotData()
    {
        CheckStreamForValidXML();
    }


}
