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
    using namespace boost::asio::ip;
    using namespace boost::asio;
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

    void Connection::HandleWrite(std::shared_ptr<std::string> /*Data*/, boost::system::error_code error)
    {        
        if(error)
        {
            CurrentConnectionState =ConnectionState::ErrorUnknown;
            std::cerr << "Handlewrite: Got socket error: " << error << " / " << error.message() << std::endl;
            return;
        }
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

        WriteTextToSocket(Stream.str());
    }

    bool Connection::verify_certificate(bool preverified,
                            boost::asio::ssl::verify_context& ctx)
    {
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.
            
        // In this example we will simply print the certificate's subject name.
#ifdef MagicCertTesting
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";
#endif
        return true;
    }
        
    bool Connection::ConnectTLSSocket()
    {
        SSLConnection = true;
        boost::system::error_code error;

        DebugOut(DebugOutputTreshold::Debug) << "Calling handshake" << std::endl;
                        
        ssl_socket.handshake(boost::asio::ssl::stream_base::client, error);
            
        if (error)
        {
            std::cerr << "Failed to handshake with error: " << error << " / " << error.message() << std::endl;
            return false;
        }
        else
        {
            DebugOut(DebugOutputTreshold::Debug) << "Succesfully upgraded to TLS!" << std::endl;
        }
            
        return true;

    }

    bool Connection::ConnectSocket()
    {
        tcp::resolver resolver(io_service);
        std::string SPortnumber;
        SPortnumber = boost::lexical_cast<string>( Portnumber);
        tcp::resolver::query query(Hostname, SPortnumber);
        boost::system::error_code io_error = boost::asio::error::host_not_found;
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;
            
        boost::asio::connect(tcp_socket.lowest_layer(), endpoint_iterator, io_error);
            
        if (io_error)
        {
            std::cerr << "Failed to connect with error: " << io_error << " / " << io_error.message() << std::endl;
            return false;
        }
        else
        {
            DebugOut(DebugOutputTreshold::Debug) << "Succesfully connected socket" << std::endl;
        }
            
        return true;
    }
        
        
    void Connection::CheckStreamForFeatures()
    {
        string str = ReadDataStream.str();
        size_t streamfeatures = str.find("</stream:features>");
            
        if(streamfeatures == string::npos)
            return;
            
        if(CurrentAuthenticationState == AuthenticationState::SASL)
        {
            ClearReadDataStream();

            CurrentConnectionState = ConnectionState::Authenticating;
            return;
        }
        if(CurrentAuthenticationState == AuthenticationState::Bind)
        {
            ClearReadDataStream();
            CurrentConnectionState = ConnectionState::Authenticating;
            BindResource();
            return;
        }
        
        // note to self: cant use loadxml() here because this is not valid xml!!!!
        // due to <stream> <stream::features></stream::features>
        xml_document xdoc;
        xdoc.load(ReadDataStream, parse_full&~parse_eol, encoding_auto);
            
        ClearReadDataStream();

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
                WriteTextToSocket(Stream.str());
                return;
            }
        }
        CurrentAuthenticationState = AuthenticationState::SASL;
            
            
        DebugOut(DebugOutputTreshold::Debug) << "SASL MANDLEBRASL" << std::endl;
        // I shall has picked an algorithm!
        
        if(FeaturesSASL_ScramSHA1)
        {
            Authentication = new  SASL::SASL_Mechanism_SCRAM_SHA1 ( boost::bind(&Connection::WriteTextToSocket, this, _1) , MyJID, Password),
            Authentication->Begin();
            return;
        }
        if(FeaturesSASL_DigestMD5)
        {
            Authentication = new  SASL::Weak::SASL_Mechanism_DigestMD5 ( boost::bind(&Connection::WriteTextToSocket, this, _1) , MyJID, Password),
            Authentication->Begin();
            return;
        }
        if(FeaturesSASL_Plain)
        {
            Authentication = new  SASL::Weak::SASL_Mechanism_PLAIN ( boost::bind(&Connection::WriteTextToSocket, this, _1) , MyJID, Password),
            Authentication->Begin();
            return;
        }
    }

    void Connection::WriteXMLToSocket(xml_document *Doc)
    {
        stringstream ss;
        Doc->save(ss, "\t", format_no_declaration);
        
        if(! (DebugOutputTreshold::Debug > DebugTreshold))
            Doc->save(std::cout, "\t", format_no_declaration);
        WriteTextToSocket(ss.str());
    }
        
    void Connection::WriteTextToSocket(const string &Data)
    {        
        std::shared_ptr<std::string> SharedData = std::make_shared<std::string>(Data);

        if(SSLConnection)
        {
            boost::asio::async_write(ssl_socket, boost::asio::buffer(*SharedData),
                                        boost::bind(&Connection::HandleWrite,this, SharedData, boost::asio::placeholders::error));
        }
        else
        {
            boost::asio::async_write(tcp_socket, boost::asio::buffer(*SharedData),
                                        boost::bind(&Connection::HandleWrite,this, SharedData, boost::asio::placeholders::error));
        }

        DebugOut(DebugOutputTreshold::Debug) << "Write text to socket:" << std::endl << Data << std::endl;

    }
        

    bool Connection::LoadXML()
    {
        string jox = ReadDataStream.str();
        istringstream TStream(jox);     
        if( IncomingDocument == NULL)
            IncomingDocument = new pugi::xml_document();
        
        xml_parse_result parseresult = IncomingDocument->load(TStream, parse_full&~parse_eol, encoding_auto);

        if(parseresult.status != xml_parse_status::status_ok)
            return false;

        ClearReadDataStream();
        
        DebugOut(DebugOutputTreshold::Debug) << std::endl << "++++++++++++++++++++++++++++++" << std::endl; 
        DebugOut(DebugOutputTreshold::Debug) << jox;
        DebugOut(DebugOutputTreshold::Debug) << std::endl << "------------------------------" << std::endl; 
        return true;
    }

    pugi::xml_node Connection::SelectSingleXMLNode(const char* xpath)
    {
        return IncomingDocument->select_single_node(xpath).node();
    }

    pugi::xpath_node_set Connection::SelectXMLNodes(const char* xpath)
    {
        return IncomingDocument->select_nodes(xpath);
    }
        
    
    void Connection::InitTLS()
    {
        DebugOut(DebugOutputTreshold::Debug) << "Server accepted to start TLS handshake" << std::endl;
        bool Success = ConnectTLSSocket();
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
        string str = ReadDataStream.str();
        size_t streamend = str.find("</stream:stream>");
        if(streamend == string::npos)
            streamend = str.find("</stream>");
            
        if(streamend == string::npos)
            return;
            
        ClearReadDataStream();

        CurrentConnectionState = ConnectionState::ErrorUnknown;

        BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            
        DebugOut(DebugOutputTreshold::Debug) << "Got stream end" << std::endl;
    }
        
    void Connection::ClearReadDataStream()
    {
        ReadDataStream.str(string(""));
    }

    void Connection::CheckForTLSProceed()
    {
        if(!LoadXML())
            return;

        if(!SelectSingleXMLNode("//proceed"))
        {
            std::cerr << "No proceed?!";

            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }

        if(CurrentAuthenticationState == AuthenticationState::StartTLS)
            InitTLS();
    }
        
    void Connection::CheckForWaitingForSession()
    {
        if(!LoadXML())
            return;

        xml_node iqnode = SelectSingleXMLNode("//iq");

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }
        
        // TODO: Verify iq response..

        string Presence = "<presence/>";

        WriteTextToSocket(Presence);
        CurrentConnectionState = ConnectionState::Connected;
        DebugOut(DebugOutputTreshold::Debug) << std::endl << "ONLINE" << std::endl;
    }

    void Connection::CheckForBindSuccess()
    {
        if(!LoadXML())
            return;

        xml_node iqnode = SelectSingleXMLNode("//iq");

        if(!iqnode)
        {
            std::cerr << "No iqnode?!";
            BrodcastConnectionState(ConnectionCallback::ConnectionState::ErrorUnknown);
            CurrentConnectionState = ConnectionState::ErrorUnknown;

            return;
        }
            
        DebugOut(DebugOutputTreshold::Debug) << std::endl<< "AUTHENTICATED" << std::endl; // todo: verify xml ;)
            
        string StartSession = "<iq type='set' id='1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
        WriteTextToSocket(StartSession);
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
        WriteTextToSocket(TStream.str());
    }

    void Connection::StartBind()
    {
        CurrentAuthenticationState = AuthenticationState::Bind;
        OpenXMPPStream();
    }

    void Connection::CheckForSASLData()
    {
        if(!LoadXML())
            return;

        xml_node challenge = SelectSingleXMLNode("//challenge");
        xml_node success = SelectSingleXMLNode("//success");

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
        if(!LoadXML())
            return;

        xpath_node message = SelectSingleXMLNode("//message");
        if(!message)
            return;

        if(StanzaHandler)
            StanzaHandler->StanzaReceived( SharedStanza(new Stanza(IncomingDocument, message.node())), shared_from_this());
        
        IncomingDocument = NULL;
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

        WriteXMLToSocket(Stanza->Document);     
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
                CheckStreamForStanza();
                break;
        default:
            break;
        }
            
        CheckForStreamEnd();
    }
        
    void Connection::HandleRead(boost::system::error_code error,
        std::size_t bytes_transferred)
    {
        if(error)
        {
            CurrentConnectionState =ConnectionState::ErrorUnknown;
            std::cerr << "HandleRead: Got socket error: " << error << " / " << error.message() << std::endl;
            return;
        }
        
        ReadDataStream.write(ReadDataBuffer, bytes_transferred);
        DebugOut(DebugOutputTreshold::Debug).write(ReadDataBuffer, bytes_transferred);
        DebugOut(DebugOutputTreshold::Debug) << flush;
        CheckStreamForValidXML();
        AsyncRead();
    }

    void Connection::AsyncRead()
    {       
        if(SSLConnection)
        {
            boost::asio::async_read(ssl_socket, boost::asio::buffer(ReadDataBuffer, ReadDataBufferSize),
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&Connection::HandleRead,this, boost::asio::placeholders::error, _2));
        }
        else
        {
            boost::asio::async_read(tcp_socket, boost::asio::buffer(ReadDataBuffer, ReadDataBufferSize),
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&Connection::HandleRead,this, boost::asio::placeholders::error, _2));
        }
    }

    Connection::Connection(const std::string &Hostname,
        int Portnumber,
        const JID &RequestedJID,
        const std::string &Password,
        StanzaCallback *StanzaHandler,
        ConnectionCallback *ConnectionHandler,
        DebugOutputTreshold DebugTreshold)
    :
        StanzaHandler(StanzaHandler),
        ConnectionHandler(ConnectionHandler),
        DebugTreshold(DebugTreshold),
        CurrentAuthenticationState(AuthenticationState::None),
        MyJID(RequestedJID),
        io_service(),
        tcp_socket(io_service),
        ssl_context(boost::asio::ssl::context::sslv23),
        ssl_socket(tcp_socket, ssl_context),
        IncomingDocument(nullptr)
    {
        FeaturesStartTLS = false;

        SSLConnection = false;

        ssl_socket.set_verify_mode(boost::asio::ssl::verify_peer);
        ssl_socket.set_verify_callback(boost::bind(&Connection::verify_certificate, this, _1, _2));

        this->Hostname = Hostname;
        this->Portnumber = Portnumber;
        this->Password = Password;
        Connect();
    }

    void Connection::Connect()
    {
        DebugOut(DebugOutputTreshold::Debug) << "Starting io_service run in background thread" << std::endl;

        PreviouslyBroadcastedState = ConnectionCallback::ConnectionState::Connecting;
        if( !ConnectSocket() )
        {
            CurrentConnectionState = ConnectionState::ErrorConnecting;
            std::cerr << "DXMPP: Failed to connect" << std::endl;
            return;
        }
        OpenXMPPStream();
        AsyncRead();

        if(IOThread == nullptr)
            IOThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service)));
    }

    Connection::~Connection()
    {
        DebugOut(DebugOutputTreshold::Debug) << "~Connection"  << std::endl;
        io_service.stop();
        while(!io_service.stopped())
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));

        if(IOThread != nullptr)
            IOThread->join();
    }


    SharedConnection Connection::Create(const std::string &Hostname,
                                   int Portnumber,
                                   const JID &RequestedJID,
                                   const std::string &Password,
                                   StanzaCallback *StanzaHandler,
                                   ConnectionCallback *ConnectionHandler,
                                   DebugOutputTreshold DebugTreshold)
    {
        return boost::shared_ptr<Connection>(
                    new Connection(Hostname,
                                   Portnumber,
                                   RequestedJID,
                                   Password,
                                   StanzaHandler,
                                   ConnectionHandler,
                                   DebugTreshold)
                );
    }
}
