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
#include <DXMPP/Network/AsyncTCPXMLClient.hpp>
#include <DXMPP/Debug/DebugOutputTreshold.hpp>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sstream>


namespace DXMPP
{
    typedef boost::shared_ptr<pugi::xml_node> SharedXMLNode;

    class Connection
            :
            public boost::enable_shared_from_this<Connection>
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

    private:

        DXMPP::Network::AsyncTCPXMLClient Client;

        DebugOutputTreshold DebugTreshold;

        bool FeaturesSASL_DigestMD5;
        bool FeaturesSASL_CramMD5;
        bool FeaturesSASL_ScramSHA1;
        bool FeaturesSASL_Plain;        
        bool FeaturesStartTLS;
        
        ConnectionState CurrentConnectionState;
        AuthenticationState CurrentAuthenticationState;
                
        std::string Hostname;
        std::string Password;
        int Portnumber;
        JID MyJID;
        

        SASL::SASLMechanism *Authentication;

        void InitTLS();
        void Connect();
        void OpenXMPPStream();
        void CheckStreamForFeatures();
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

        void ClientDisconnected();
        void ClientGotData();
                

        void BrodcastConnectionState(ConnectionCallback::ConnectionState NewState);

        Connection(const std::string &Hostname,
            int Portnumber,
            const JID &RequestedJID,
            const std::string &Password,
            StanzaCallback *StanzaHandler = nullptr,
            ConnectionCallback *ConnectionHandler = nullptr,
            DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error);



    public:
        SharedStanza CreateStanza(const JID &Target);
        void SendStanza(SharedStanza Stanza);
        
        static SharedConnection Create(const std::string &Hostname,
                                       int Portnumber,
                                       const JID &RequestedJID,
                                       const std::string &Password,
                                       StanzaCallback *StanzaHandler = nullptr,
                                       ConnectionCallback *ConnectionHandler = nullptr,
                                       DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error);
        
        ~Connection();
    };
}
#endif
