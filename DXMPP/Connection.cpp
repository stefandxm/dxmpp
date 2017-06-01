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

    std::atomic<int> Connection::ReconnectionCount(-1);

    void Connection::BrodcastConnectionState(ConnectionCallback::ConnectionState NewState)
    {
        if(PreviouslyBroadcastedState == NewState)
            return;
        PreviouslyBroadcastedState = NewState;


        boost::shared_lock<boost::shared_mutex> ReadLock(ConnectionHandlerMutex);

        if(!ConnectionHandler)
            return;

        ConnectionHandler->ConnectionStateChanged(NewState, shared_from_this());
    }


    void Connection::OpenXMPPStream()
    {
        CurrentConnectionState = ConnectionState::WaitingForFeatures;

        stringstream Stream;
        Stream << "<?xml version='1.0' encoding='utf-8'?>";
        Stream << "<stream:stream";
        Stream << " from = '" << MyJID.GetBareJID() << "'";
        Stream << " to = '" << MyJID.GetDomain() << "'";
        Stream << " version='1.0'";
        Stream << " xml:lang='en'";
        Stream << " xmlns='jabber:client'";
        Stream << " xmlns:stream='http://etherx.jabber.org/streams'>";

        DebugOut(DebugOutputTreshold::Debug)
            << "DXMPP: Opening stream" << std::endl;// << Stream.str();

        Client->WriteTextToSocket(Stream.str());
    }

    void Connection::CheckStreamForFeatures()
    {
        string str = Client->ReadDataStream->str();
        size_t streamfeatures = str.find("</stream:features>");

        if(streamfeatures == string::npos)
            return;

        if(CurrentAuthenticationState == AuthenticationState::SASL)
        {
            Client->ClearReadDataStream();
            CurrentConnectionState = ConnectionState::Authenticating;
            return;
        }
        if(CurrentAuthenticationState == AuthenticationState::Bind)
        {
            Client->ClearReadDataStream();
            CurrentConnectionState = ConnectionState::Authenticating;
            BindResource();
            return;
        }

        // note to self: cant use loadxml() here because this is not valid xml!!!!
        // due to <stream> <stream::features></stream::features>
        xml_document xdoc;
        xdoc.load(*Client->ReadDataStream, parse_full&~parse_eol, encoding_auto);

        Client->ClearReadDataStream();

        ostringstream o;

        xdoc.save(o, "\t", format_no_declaration);

        pugi::xpath_node starttls = xdoc.select_single_node("//starttls");
        if(starttls)
        {
            DebugOut(DebugOutputTreshold::Debug)
                << std::endl << "START TLS SUPPORTED" << std::endl;
            FeaturesStartTLS = true;
        }

        //Move to sasl class
        pugi::xpath_node_set mechanisms = xdoc.select_nodes("//mechanism");
        for (auto it = mechanisms.begin(); it != mechanisms.end(); it++)
        {
            xml_node node = it->node();
            string mechanism = string(node.child_value());
            DebugOut(DebugOutputTreshold::Debug)
                << "Mechanism supported: " << mechanism << std::endl;


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
                Client->WriteTextToSocket(Stream.str());
                return;
            }
        }
        CurrentAuthenticationState = AuthenticationState::SASL;


        DebugOut(DebugOutputTreshold::Debug) << "SASL MANDLEBRASL" << std::endl;
        // I shall has picked an algorithm!

        if(FeaturesSASL_ScramSHA1)
        {
            Authentication.reset(new  SASL::SASL_Mechanism_SCRAM_SHA1 ( Client, MyJID, Password)),
                    Authentication->Begin();
            return;
        }
        if(FeaturesSASL_DigestMD5)
        {
            Authentication.reset(new  SASL::Weak::SASL_Mechanism_DigestMD5 ( Client , MyJID, Password)),
                    Authentication->Begin();
            return;
        }
        if(FeaturesSASL_Plain)
        {
            Authentication.reset(new  SASL::Weak::SASL_Mechanism_PLAIN ( Client , MyJID, Password)),
                    Authentication->Begin();
            return;
        }
    }


    void Connection::InitTLS()
    {
        DebugOut(DebugOutputTreshold::Debug)
            << "Server accepted to start TLS handshake" << std::endl;
        bool Success = Client->ConnectTLSSocket();
        if(Success)
        {
            DebugOut(DebugOutputTreshold::Debug)
                << "TLS Connection successfull. Reopening stream." << std::endl;
            OpenXMPPStream();
        }
        else
        {
            std::cerr << "TLS Connection failed" << std::endl;
            CurrentConnectionState = ConnectionState::ErrorUnknown;
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
        }

    }

    // Explicit string hax
    void Connection::CheckForStreamEnd()
    {
        string str = Client->ReadDataStream->str();
        size_t streamend = str.find("</stream:stream>");
        if(streamend == string::npos)
            streamend = str.find("</stream>");

        if(streamend == string::npos)
            return;

        std::cerr << "Got end of stream from xmppserver" << std::endl;
        Client->ClearReadDataStream();

        CurrentConnectionState = ConnectionState::ErrorUnknown;
        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);

        DebugOut(DebugOutputTreshold::Debug) << "Got stream end" << std::endl;
    }

    void Connection::CheckForTLSProceed(pugi::xml_document* Doc)
    {
        if(!Doc->select_single_node("//proceed").node())
        {
            std::cerr << "No proceed tag; B0rked SSL?!";

            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;
            return;
        }

        if(CurrentAuthenticationState == AuthenticationState::StartTLS)
            InitTLS();
    }

    void Connection::CheckForWaitingForSession(pugi::xml_document* Doc)
    {
        xml_node iqnode = Doc->select_single_node("//iq").node();

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }

        // TODO: Verify iq response..

        string Presence = "<presence/>";

        Client->WriteTextToSocket(Presence);
        CurrentConnectionState = ConnectionState::Connected;
        Client->SetKeepAliveByWhiteSpace(string(" "), 5);
        DebugOut(DebugOutputTreshold::Debug) << std::endl << "ONLINE" << std::endl;
    }

    void Connection::CheckForBindSuccess(pugi::xml_document* Doc)
    {
        xml_node iqnode = Doc->select_single_node("//iq").node();

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }

        DebugOut(DebugOutputTreshold::Debug)
                << std::endl
                << "AUTHENTICATED"
                << std::endl; // todo: verify xml ;)

        string StartSession = "<iq type='set' id='1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
        Client->WriteTextToSocket(StartSession);
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
        Client->WriteTextToSocket(TStream.str());
    }

    void Connection::StartBind()
    {
        CurrentAuthenticationState = AuthenticationState::Bind;
        OpenXMPPStream();
    }

    void Connection::CheckForSASLData(pugi::xml_document* Doc)
    {
        xml_node challenge = Doc->select_single_node("//challenge").node();
        xml_node success = Doc->select_single_node("//success").node();

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
            DebugOut(DebugOutputTreshold::Debug)
                    <<
                       std::endl
                    << "Authentication succesfull."
                    << std::endl;
            StartBind();
        }
    }

    void Connection::CheckStreamForAuthenticationData(pugi::xml_document* Doc)
    {
        switch(CurrentAuthenticationState)
        {
            case AuthenticationState::StartTLS:
                CheckForTLSProceed(Doc);
                break;
            case AuthenticationState::SASL:
                CheckForSASLData(Doc);
                break;
            case AuthenticationState::Bind:
                CheckForBindSuccess(Doc);
                break;
            case AuthenticationState::Authenticated:
                break;
        default:
            break;
        }

    }

    bool Connection::CheckStreamForStanza(pugi::xml_document* Doc)
    {
        xml_node message = Doc->select_single_node("//message").node();

        if(!message)
            return false;

        return true;
    }

    void Connection::DispatchStanza(std::unique_ptr<pugi::xml_document> Doc)
    {
        xml_node message = Doc->select_single_node("//message").node();
        boost::shared_lock<boost::shared_mutex> ReadLock(StanzaHandlerMutex);
        if(StanzaHandler)
            StanzaHandler->StanzaReceived(
                        SharedStanza(
                            new Stanza( std::move(Doc),
                                       message)),
                        shared_from_this());
    }

    void Connection::CheckForPresence(pugi::xml_document* Doc)
    {
        xml_node presence = Doc->select_single_node("//presence").node();

        if(!presence)
            return;

        Roster->OnPresence(presence);
    }

    void Connection::CheckForIQ(pugi::xml_document* Doc)
    {
        xml_node iq = Doc->select_single_node("//iq").node();

        if(!iq)
            return;

        Roster->OnIQ(iq);
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
        if(this->CurrentConnectionState != ConnectionState::Connected)
        {
            throw std::runtime_error("Trying to send Stanza with disconnected connection.");
        }

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
        Stanza->Message.attribute("id").set_value( Stanza->ID.c_str() );

        Client->WriteXMLToSocket(Stanza->Document.get());
    }

    void Connection::CheckStreamForValidXML()
    {
        if(CurrentConnectionState == ConnectionState::WaitingForFeatures)
        {
            BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
            CheckStreamForFeatures();
            return;
        }


        std::unique_ptr<pugi::xml_document> Doc;
        int NrFetched = 0;
        int Iteration = 0;

        bool LoadMore = true;
        do//while(Client->LoadXML(Iteration++))
        {
            if(LoadMore)
                LoadMore = Client->LoadXML(Iteration++);

            Doc = Client->FetchDocument();
            if(Doc == nullptr)
                break;

            NrFetched++;

            switch(CurrentConnectionState)
            {
                case ConnectionState::WaitingForSession:
                    BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
                    CheckForWaitingForSession(Doc.get());
                    break;
                case ConnectionState::WaitingForFeatures:
                    break;
                case ConnectionState::Authenticating:
                    BrodcastConnectionState(ConnectionCallback::ConnectionState::Connecting);
                    CheckStreamForAuthenticationData(Doc.get());
                    break;
                case ConnectionState::Connected:
                    BrodcastConnectionState(ConnectionCallback::ConnectionState::Connected);
                    CheckForPresence(Doc.get());
                    CheckForIQ(Doc.get());
                    if(CheckStreamForStanza(Doc.get()))
                    {
                        DispatchStanza(std::move(Doc));
                    }
                    break;
            default:
                break;
            }

        }while(true);

        CheckForStreamEnd();
    }

    void Connection::Reset()
    {
        FeaturesSASL_CramMD5 = false;
        FeaturesSASL_DigestMD5 = false;
        FeaturesSASL_Plain = false;
        FeaturesSASL_ScramSHA1 = false;
        FeaturesStartTLS = false;
        CurrentAuthenticationState = AuthenticationState::None;

        if(Authentication != nullptr)
        {
            Authentication.reset();
        }

    }

    void Connection::Reconnect()
    {
        ReconnectionCount++;
        Reset();
        Connect();
    }

    Connection::Connection(const std::string &Hostname,
        int Portnumber,
        const JID &RequestedJID,
        const std::string &Password,
        ConnectionCallback *ConnectionHandler,
        StanzaCallback *StanzaHandler,
        PresenceCallback *PresenceHandler,
        IQCallback *IQHandler,
        SubscribeCallback *SubscribeHandler,
        SubscribedCallback *SubscribedHandler,
        UnsubscribedCallback *UnsubscribedHandler,
        TLSVerification *Verification,
        TLSVerificationMode VerificationMode,
        DebugOutputTreshold DebugTreshold)
    :
        Disposing(false),
        SelfHostedVerifier(new TLSVerification(VerificationMode)),
        ConnectionHandler(ConnectionHandler),
        StanzaHandler(StanzaHandler),
        DebugTreshold(DebugTreshold),
        CurrentAuthenticationState(AuthenticationState::None),
        Hostname(Hostname),
        Password(Password),
        Portnumber(Portnumber),
        MyJID(RequestedJID),
        Verification(Verification),
        VerificationMode(VerificationMode),
        Authentication(nullptr)
    {
        Roster = new RosterMaintaner (nullptr,
               PresenceHandler,
               SubscribeHandler,
               SubscribedHandler,
               UnsubscribedHandler,
               IQHandler);

        //this->Password = Password;

        Reconnect();
    }

    void Connection::Connect()
    {
        DebugOut(DebugOutputTreshold::Debug)
                << "Starting io_service run in background thread"
                << std::endl;

        if( io_service != nullptr )
        {
            io_service->stop();
        }

        if( IOThread != nullptr )
        {
            IOThread->join();
        }


        if (Client != nullptr)
        {
            this->Roster->ResetClient(nullptr);
            if (this->Authentication != nullptr)
            {
                this->Authentication.reset();
            }
        }


        io_service.reset( new boost::asio::io_service() );

        Client.reset(
                    new Network::AsyncTCPXMLClient (
                        io_service,
                        ((Verification != nullptr) ? Verification : SelfHostedVerifier.get()),
                        Hostname,
                        Portnumber,
                        boost::bind(&Connection::ClientDisconnected, this),
                        boost::bind(&Connection::ClientGotData, this),
                        DebugTreshold ) );


        PreviouslyBroadcastedState = ConnectionCallback::ConnectionState::Connecting;
        //Client->Reset();
        if( !Client->ConnectSocket() )
        {
            CurrentConnectionState = ConnectionState::ErrorConnecting;
            std::cerr << "DXMPP: Failed to connect" << std::endl;
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorConnecting);
            return;
        }
        OpenXMPPStream();
        Client->AsyncRead();
        Roster->ResetClient(Client);

        // Fork io
        IOThread.reset(
                    new boost::thread(boost::bind(
                                          &Connection::Run,
                                          this)));
    }

    void Connection::Run()
    {

        DebugOut(DebugOutputTreshold::Debug)
                << "DXMPP: Starting io run" << std::endl;
        try
        {
            io_service->run();
        }
        catch(const std::exception &ex)
        {
            DebugOut(DebugOutputTreshold::Error)
                    << "DXMPP IO Exception: " << ex.what() << std::endl;

        }
        catch(...)
        {
            DebugOut(DebugOutputTreshold::Error)
                    << "DXMPP IO Exception: Unknown" << std::endl;
        }
        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
    }



    Connection::~Connection()
    {
        Disposing=true;

        //std::cout << "~Connection" << std::endl;
        if( Authentication != nullptr )
        {
            //std::cout << "Delete authentication" << std::endl;
            Authentication.reset();
        }
        //std::cout << "Delete roster" << std::endl;
        delete Roster;

        if( io_service != nullptr )
        {
            //std::cout << "Stop IO Service" << std::endl;
            io_service->stop();
        }

        io_service.reset();
        //std::cout << "Client = nullptr" << std::endl;
        Client.reset();

        if( IOThread != nullptr )
        {
            //std::cout << "Join IO Thread" << std::endl;
            IOThread->join();
        }

        //std::cout << "~Connection done" << std::endl;
    }

    SharedConnection Connection::Create( const std::string &Hostname,
                                         int Portnumber,
                                         const JID &RequestedJID,
                                         const std::string &Password,
                                         IEventHandler* Handler,
                                         TLSVerification *Verification,
                                         DebugOutputTreshold DebugTreshold)
    {
        ConnectionCallback *ConnectionHandler = dynamic_cast<ConnectionCallback*>  (Handler);
        StanzaCallback *StanzaHandler = dynamic_cast<StanzaCallback*> (Handler);
        PresenceCallback *PresenceHandler = dynamic_cast<PresenceCallback*>(Handler);
        IQCallback *IQHandler = dynamic_cast<IQCallback*>(Handler);
        SubscribeCallback *SubscribeHandler = dynamic_cast<SubscribeCallback*>(Handler);
        SubscribedCallback *SubscribedHandler = dynamic_cast<SubscribedCallback*>(Handler);
        UnsubscribedCallback *UnsubscribedHandler = dynamic_cast<UnsubscribedCallback*>(Handler);


        return boost::shared_ptr<Connection>(
                    new Connection(Hostname,
                                   Portnumber,
                                   RequestedJID,
                                   Password,
                                   ConnectionHandler,
                                   StanzaHandler,
                                   PresenceHandler,
                                   IQHandler,
                                   SubscribeHandler,
                                   SubscribedHandler,
                                   UnsubscribedHandler,
                                   Verification,
                                   Verification->Mode,
                                   DebugTreshold) );
    }


    SharedConnection Connection::Create(const std::string &Hostname,
                                        int Portnumber,
                                        const JID &RequestedJID,
                                        const std::string &Password,
                                        IEventHandler* Handler,
                                        TLSVerificationMode VerificationMode,
                                        DebugOutputTreshold DebugTreshold)
    {
        ConnectionCallback *ConnectionHandler = dynamic_cast<ConnectionCallback*>  (Handler);
        StanzaCallback *StanzaHandler = dynamic_cast<StanzaCallback*> (Handler);
        PresenceCallback *PresenceHandler = dynamic_cast<PresenceCallback*>(Handler);
        IQCallback *IQHandler = dynamic_cast<IQCallback*>(Handler);
        SubscribeCallback *SubscribeHandler = dynamic_cast<SubscribeCallback*>(Handler);
        SubscribedCallback *SubscribedHandler = dynamic_cast<SubscribedCallback*>(Handler);
        UnsubscribedCallback *UnsubscribedHandler = dynamic_cast<UnsubscribedCallback*>(Handler);

        if( ConnectionHandler == nullptr)
            std::cerr << "ConnectionHandler is null" << std::endl;


        return boost::shared_ptr<Connection>(
                    new Connection(Hostname,
                                   Portnumber,
                                   RequestedJID,
                                   Password,
                                   ConnectionHandler,
                                   StanzaHandler,
                                   PresenceHandler,
                                   IQHandler,
                                   SubscribeHandler,
                                   SubscribedHandler,
                                   UnsubscribedHandler,
                                   nullptr,
                                   VerificationMode,
                                   DebugTreshold) );
    }

    void Connection::ClientDisconnected()
    {
        //std::cerr << "Client disconnected." << std::endl;
        CurrentConnectionState  = ConnectionState::ErrorUnknown;
        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
    }
    void Connection::ClientGotData()
    {
        CheckStreamForValidXML();
    }


}
