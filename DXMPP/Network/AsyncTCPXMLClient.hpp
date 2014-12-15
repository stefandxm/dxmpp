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

#include <pugixml/pugixml.hpp>
#include <DXMPP/Debug/DebugOutputTreshold.hpp>
#include <DXMPP/TLSVerification.hpp>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr.hpp>
#include <memory>
#include <sstream>

namespace DXMPP
{
    namespace Network
    {
        class AsyncTCPXMLClient
        {
            TLSVerification *TLSConfig;

            DebugOutputTreshold DebugTreshold;
            bool HasUnFetchedXML;

            static const int ReadDataBufferSize = 1024;
            char ReadDataBufferNonSSL[ReadDataBufferSize];
            std::stringstream ReadDataStreamNonSSL;

            char ReadDataBufferSSL[ReadDataBufferSize];
            std::stringstream ReadDataStreamSSL;

            boost::asio::mutable_buffers_1 SSLBuffer;
            boost::asio::mutable_buffers_1 NonSSLBuffer;
            void SendKeepAliveWhitespace();
            std::unique_ptr<boost::asio::deadline_timer>  SendKeepAliveWhitespaceTimer;
            std::string SendKeepAliveWhiteSpaceDataToSend;
            int SendKeepAliveWhiteSpaceTimeeoutSeconds;
            
            boost::shared_mutex WriteMutex;
            boost::posix_time::ptime LastWrite;


        public:

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

            //boost::scoped_ptr<boost::thread> IOThread;
            boost::shared_ptr<boost::asio::io_service> io_service;
            boost::scoped_ptr<boost::asio::ssl::context> ssl_context;
            
            boost::scoped_ptr<boost::asio::ip::tcp::socket> tcp_socket;
            
            boost::scoped_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> ssl_socket;


            void HandleWrite(boost::asio::ip::tcp::socket *active_socket,
                             std::shared_ptr<std::string> Data,
                             const boost::system::error_code &error);


            void SetKeepAliveByWhiteSpace(const std::string &DataToSend,
                                          int TimeoutSeconds = 30);
            bool EnsureTCPKeepAlive();
            bool ConnectTLSSocket();
            bool ConnectSocket();

            bool VerifyCertificate(bool preverified,
                                    boost::asio::ssl::verify_context& ctx);

            void WriteXMLToSocket(pugi::xml_document *Doc);
            void WriteTextToSocket(const std::string &Data);

            void HandleRead(
                            boost::asio::ip::tcp::socket *active_socket,
                            char *ActiveDataBuffer,
                            const boost::system::error_code &error,
                            std::size_t bytes_transferred);
            void AsyncRead();

            pugi::xml_document *IncomingDocument;

            bool LoadXML();
            pugi::xml_node SelectSingleXMLNode(const char* xpath);
            pugi::xpath_node_set SelectXMLNodes(const char* xpath);

            std::unique_ptr<pugi::xml_document>  FetchDocument();

            void ClearReadDataStream();

            //void ForkIO();
            
            void Reset();


            ~AsyncTCPXMLClient()
            {
                /*
                io_service.stop();
                while(!io_service.stopped())
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
                    */
                /*
                
                if(IOThread != nullptr)
                {
                    if( boost::this_thread::get_id() != IOThread->get_id ())
                        IOThread->join();
                }
                
                */
            }


            typedef boost::function<void (void)> ErrorCallbackFunction;
            typedef boost::function<void (void)> GotDataCallbackFunction;

            ErrorCallbackFunction ErrorCallback;
            GotDataCallbackFunction GotDataCallback;

            AsyncTCPXMLClient( 
                               boost::shared_ptr<boost::asio::io_service> IOService,
                               TLSVerification *TLSConfig,
                               const std::string &Hostname,
                               int Portnumber,                               
                               const ErrorCallbackFunction &ErrorCallback,
                               const GotDataCallbackFunction &GotDataCallback,
                               DebugOutputTreshold DebugTreshold = DebugOutputTreshold::Error)
                :                  
                  TLSConfig(TLSConfig),
                  DebugTreshold(DebugTreshold),
                  HasUnFetchedXML(false),
                  SSLBuffer( boost::asio::buffer(ReadDataBufferSSL, ReadDataBufferSize) ),
                  NonSSLBuffer( boost::asio::buffer(ReadDataBufferNonSSL, ReadDataBufferSize) ),
                  IncomingDocument(nullptr),
                  ErrorCallback(ErrorCallback),
                  GotDataCallback(GotDataCallback)

            {                
                this->io_service = IOService;
                this->Hostname = Hostname;
                this->Portnumber = Portnumber;
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
