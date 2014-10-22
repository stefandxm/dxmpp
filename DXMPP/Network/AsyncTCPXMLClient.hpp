//
//  AsyncTCPXMLClient.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_AsyncTCPClient_hpp
#define DXMPP_AsyncTCPClient_hpp

#include <DXMPP/pugixml/pugixml.hpp>
#include <DXMPP/Debug/DebugOutputTreshold.hpp>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sstream>

namespace DXMPP
{
    namespace Network
    {
        class AsyncTCPXMLClient
        {
            DebugOutputTreshold DebugTreshold;

        public:
            enum class ConnectionState
            {
                Connected,
                Disconnected,
                Error
            };

            std::string Hostname;
            int Portnumber;

            bool SSLConnection;
            ConnectionState CurrentConnectionState;

            boost::scoped_ptr<boost::thread> IOThread;
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::socket tcp_socket;
            boost::asio::ssl::context ssl_context;
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> ssl_socket;

            static const int ReadDataBufferSize = 1024;
            char ReadDataBuffer[ReadDataBufferSize];
            std::stringstream ReadDataStream;


            void HandleWrite(std::shared_ptr<std::string> Data, boost::system::error_code error);

            bool ConnectTLSSocket();
            bool ConnectSocket();

            bool verify_certificate(bool preverified,
                                    boost::asio::ssl::verify_context& ctx);

            void WriteXMLToSocket(pugi::xml_document *Doc);
            void WriteTextToSocket(const std::string &Data);

            void HandleRead(boost::system::error_code error, std::size_t bytes_transferred);
            void AsyncRead();

            pugi::xml_document *IncomingDocument;

            bool LoadXML();
            pugi::xml_node SelectSingleXMLNode(const char* xpath);
            pugi::xpath_node_set SelectXMLNodes(const char* xpath);

            pugi::xml_document *FetchDocument();

            void ClearReadDataStream();

            void ForkIO();


            virtual ~AsyncTCPXMLClient()
            {
                io_service.stop();
                while(!io_service.stopped())
                    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

                if(IOThread != nullptr)
                    IOThread->join();
            }


            typedef boost::function<void (void)> ErrorCallbackFunction;
            typedef boost::function<void (void)> GotDataCallbackFunction;

            ErrorCallbackFunction ErrorCallback;
            GotDataCallbackFunction GotDataCallback;

            AsyncTCPXMLClient( const std::string &Hostname,
                               int Portnumber,
                               const ErrorCallbackFunction &ErrorCallback,
                               const GotDataCallbackFunction &GotDataCallback,
                               DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error)
                :
                  DebugTreshold(DebugTreshold),
                  io_service(),
                  tcp_socket(io_service),
                  ssl_context(boost::asio::ssl::context::sslv23),
                  ssl_socket(tcp_socket, ssl_context),
                  IncomingDocument(nullptr),
                  ErrorCallback(ErrorCallback),
                  GotDataCallback(GotDataCallback)

            {
                SSLConnection = false;
                CurrentConnectionState = ConnectionState::Disconnected;

                ssl_socket.set_verify_mode(boost::asio::ssl::verify_peer);
                ssl_socket.set_verify_callback(boost::bind(&AsyncTCPXMLClient::verify_certificate, this, _1, _2));

                this->Hostname = Hostname;
                this->Portnumber = Portnumber;
            }
        };
    }
}

#endif // DXMPP_AsyncTCPClient_hpp
