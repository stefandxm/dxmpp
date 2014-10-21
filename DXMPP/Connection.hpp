//
//  Connection.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_Connection_hpp
#define DXMPP_Connection_hpp

#include <DXMPP/pugixml/pugixml.hpp>
#include <DXMPP/JID.hpp>
#include <DXMPP/SASL/SASLMechanism.hpp>
#include <DXMPP/Stanza.hpp>
#include <DXMPP/StanzaCallback.hpp>
#include <DXMPP/ConnectionCallback.hpp>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sstream>


namespace DXMPP
{
    typedef boost::shared_ptr<pugi::xml_node> SharedXMLNode;

    class Connection : public boost::enable_shared_from_this<Connection>
	{
        enum class ConnectionState
        {
            NotConnected = 0,
            Connecting,
            WaitingForFeatures,
            Authenticating,
            WaitingForSession,
            Connected,

            ErrorConnecting,
            ErrorAuthenticating,
            ErrorUnknown
        };

        enum class AuthenticationState
        {
            None,
            StartTLS,
            SASL,
            Bind,
            Authenticated
        };


		StanzaCallback *StanzaHandler;
		ConnectionCallback *ConnectionHandler;
		ConnectionCallback::ConnectionState PreviouslyBroadcastedState;

	public:
		enum class DebugOutputTreshold
		{
			None = 0,
			Error = 1,
			Debug = 2,
		};

	private:
        
        bool FeaturesSASL_DigestMD5;
        bool FeaturesSASL_CramMD5;
        bool FeaturesSASL_ScramSHA1;
        bool FeaturesSASL_Plain;        
		DebugOutputTreshold DebugTreshold;
		
		bool SSLConnection;
		bool FeaturesStartTLS;
		
		
		ConnectionState CurrentConnectionState;
		AuthenticationState CurrentAuthenticationState;
				
		std::string Hostname;
		std::string Password;
		int Portnumber;
		
		JID MyJID;
		
        boost::thread *IOThread;
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket tcp_socket;
		boost::asio::ssl::context ssl_context;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket;

		SASL::SASLMechanism *Authentication;

        void HandleWrite(std::shared_ptr<std::string> Data, boost::system::error_code error);

		void OpenXMPPStream();
		bool verify_certificate(bool preverified,
								boost::asio::ssl::verify_context& ctx);		
		bool ConnectTLSSocket();
		bool ConnectSocket();
		static const int ReadDataBufferSize = 1024;
		char ReadDataBuffer[ReadDataBufferSize];		
		std::stringstream ReadDataStream;
		
		void CheckStreamForFeatures();
		void WriteXMLToSocket(pugi::xml_document *Doc);
		void WriteTextToSocket(const std::string &Data);	
		void InitTLS();	

		pugi::xml_document *IncomingDocument;


		bool LoadXML();	
		pugi::xml_node SelectSingleXMLNode(const char* xpath);
		pugi::xpath_node_set SelectXMLNodes(const char* xpath);

		void ClearReadDataStream();

		// who vomited?
		void CheckForTLSProceed();
		void CheckForWaitingForSession();
		void CheckForBindSuccess();
		void CheckForSASLData();
		void CheckStreamForAuthenticationData();		
		void CheckStreamForStanza();

		// this is ok (invalid xml)
		void CheckForStreamEnd();

		// this is ok
		void CheckStreamForValidXML();

		void BindResource();
		void StartBind();
				
		void HandleRead(boost::system::error_code error, std::size_t bytes_transferred);
		void AsyncRead();
		void Connect();

		void BrodcastConnectionState(ConnectionCallback::ConnectionState NewState);

        Connection(const std::string &Hostname,
            int Portnumber,
            const JID &RequestedJID,
            const std::string &Password,
            StanzaCallback *StanzaHandler = nullptr,
            ConnectionCallback *ConnectionHandler = nullptr,
            DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error)
        :
            StanzaHandler(StanzaHandler),
            ConnectionHandler(ConnectionHandler),
            DebugTreshold(DebugTreshold),
            CurrentAuthenticationState(AuthenticationState::None),
            MyJID(RequestedJID),
            IOThread(nullptr),
            io_service(),
            tcp_socket(io_service),
            ssl_context(boost::asio::ssl::context::sslv23),
            ssl_socket(tcp_socket, ssl_context),
            IncomingDocument(NULL)
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



    public:
		SharedStanza CreateStanza(const JID &Target);
		void SendStanza(SharedStanza Stanza);
		
        static SharedConnection Create(const std::string &Hostname,
                                       int Portnumber,
                                       const JID &RequestedJID,
                                       const std::string &Password,
                                       StanzaCallback *StanzaHandler = nullptr,
                                       ConnectionCallback *ConnectionHandler = nullptr,
                                       DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error)
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
		
        ~Connection();
	};
}
#endif
