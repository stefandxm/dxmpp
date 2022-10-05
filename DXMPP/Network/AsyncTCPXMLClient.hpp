//
//  AsyncTCPXMLClient.hpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_AsyncTCPClient_hpp
#define DXMPP_AsyncTCPClient_hpp

#ifdef __APPLE__

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostics push
#pragma GCC diagnostics ignored "-Wdeprecated-declarations"
#endif // __clang__


#endif

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr.hpp>
#include <memory>
#include <sstream>
#include <queue>
#include <iostream>

#include <pugixml/pugixml.hpp>
#include <DXMPP/Debug/DebugOutputTreshold.hpp>
#include <DXMPP/TLSVerification.hpp>


namespace DXMPP
{
    namespace Network
    {
        class AsyncTCPXMLClient
        {
            TLSVerification *TLSConfig;
            DebugOutputTreshold DebugTreshold;
            boost::asio::const_buffer Certificate;
            boost::asio::const_buffer Privatekey;

            static const int ReadDataBufferSize = 1024;
            char ReadDataBufferNonSSL[ReadDataBufferSize];
            std::stringstream ReadDataStreamNonSSL;

            char ReadDataBufferSSL[ReadDataBufferSize];
            std::stringstream ReadDataStreamSSL;

            boost::asio::mutable_buffer SSLBuffer;
            boost::asio::mutable_buffer NonSSLBuffer;
            void SendKeepAliveWhitespace();
            std::unique_ptr<boost::asio::deadline_timer>  SendKeepAliveWhitespaceTimer;
            std::string SendKeepAliveWhiteSpaceDataToSend;
            int SendKeepAliveWhiteSpaceTimeeoutSeconds;

            boost::asio::io_context::strand SynchronizationStrand;

            //boost::shared_mutex ReadMutex;
            //boost::shared_mutex WriteMutex;
            boost::posix_time::ptime LastWrite;

            boost::shared_mutex IncomingDocumentsMutex;
            std::queue<pugi::xml_document*> IncomingDocuments;

            std::queue<std::shared_ptr<std::string>> OutgoingData;
            boost::shared_mutex OutgoingDataMutex;
            bool Flushing;

            void FlushOutgoingDataUnsafe();

        public:

            void FlushOutgoingData();

            void SignalError();
            std::stringstream *ReadDataStream;
            char * ReadDataBuffer;

            enum class ConnectionState
            {
                Connected,
                Upgrading,
                Disconnected,
                Error
            };

            std::string Hostname;
            int Portnumber;

            bool SSLConnection;
            volatile ConnectionState CurrentConnectionState;

            std::shared_ptr<boost::asio::io_service> io_service;
            std::unique_ptr<boost::asio::ssl::context> ssl_context;
            std::unique_ptr<boost::asio::ip::tcp::socket> tcp_socket;
            std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> ssl_socket;


            void HandleWrite(boost::asio::ip::tcp::socket *active_socket,
                             std::shared_ptr<std::string> Data,
                             const boost::system::error_code &error);


            void SetKeepAliveByWhiteSpace(const std::string &DataToSend,
                                          int TimeoutSeconds = 5);
            bool EnsureTCPKeepAlive();
            bool ConnectTLSSocket();
            bool ConnectSocket();
            void AsyncRead();

            bool VerifyCertificate(bool preverified,
                                    boost::asio::ssl::verify_context& ctx);

            void WriteXMLToSocket(pugi::xml_document *Doc);
            void WriteTextToSocket(const std::string &Data);

            void HandleRead(
                            boost::asio::ip::tcp::socket *active_socket,
                            char *ActiveDataBuffer,
                            const boost::system::error_code &error,
                            std::size_t bytes_transferred);


            bool InnerLoadXML();
            bool LoadXML(int Iteration );
            void LoadXML();

            std::unique_ptr<pugi::xml_document>  FetchDocument();

            void ClearReadDataStream();

            void Reset();


            ~AsyncTCPXMLClient()
            {
                std::cout << "~AsyncTCPXMLClient" << std::endl;
                try
                {
                    if( SendKeepAliveWhitespaceTimer != nullptr )
                        SendKeepAliveWhitespaceTimer->cancel();
                }
                catch( std::exception & ex )
                { std::cout << "Exception: " << ex.what() << std::endl; }
                try
                {
                    io_service->stop();
                    while (!io_service->stopped())
                    {
                    }
                }
                catch( std::exception & ex )
                { std::cout << "Exception: " << ex.what() << std::endl; }
                try
                { ssl_socket->shutdown(); }
                catch( std::exception & ex )
                { std::cout << "Exception: " << ex.what() << std::endl; }
                try
                { tcp_socket->close(); }
                catch( std::exception & ex )
                { std::cout << "Exception: " << ex.what() << std::endl; }

                while (!IncomingDocuments.empty())
                {
                    auto n = IncomingDocuments.front();
                    delete n;
                    IncomingDocuments.pop();
                }

            }


            typedef boost::function<void (void)> ErrorCallbackFunction;
            typedef boost::function<void (void)> GotDataCallbackFunction;

            ErrorCallbackFunction ErrorCallback;
            GotDataCallbackFunction GotDataCallback;

            AsyncTCPXMLClient(
                               std::shared_ptr<boost::asio::io_service> IOService,
                               boost::asio::const_buffer Certificate,
                               boost::asio::const_buffer Privatekey,
                               TLSVerification *TLSConfig,
                               const std::string &Hostname,
                               int Portnumber,
                               const ErrorCallbackFunction &ErrorCallback,
                               const GotDataCallbackFunction &GotDataCallback,
                               DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error)
                :
                  TLSConfig(TLSConfig),
                  Certificate(Certificate),
                  Privatekey(Privatekey),
                  DebugTreshold(DebugTreshold),
                  SSLBuffer( boost::asio::buffer(ReadDataBufferSSL, ReadDataBufferSize) ),
                  NonSSLBuffer( boost::asio::buffer(ReadDataBufferNonSSL, ReadDataBufferSize) ),
                  SynchronizationStrand(*IOService),
                  ErrorCallback(ErrorCallback),
                  GotDataCallback(GotDataCallback)

            {
                this->io_service = IOService;
                this->Hostname = Hostname;
                this->Portnumber = Portnumber;
                Flushing = false;
            }
        };
    }
}

#ifdef __APPLE__

#if defined(__clang__)
#pragma clang diagnostic pop

#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif // __clang__


#endif


#endif // DXMPP_AsyncTCPClient_hpp
