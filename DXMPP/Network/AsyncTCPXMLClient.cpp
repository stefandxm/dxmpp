//
//  AsyncTCPXMLClient.cpp
//  DXMPP
//
//  Created by Stefan Karlsson 2014
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#include <DXMPP/Network/AsyncTCPXMLClient.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>

namespace DXMPP
{
    namespace Network
    {
#define DebugOut(DebugLevel) \
if (DebugLevel > DebugTreshold) {} \
else std::cout

        using namespace std;
        using namespace boost::asio::ip;
        using namespace boost::asio;
        using namespace pugi;


        std::unique_ptr<pugi::xml_document> AsyncTCPXMLClient::FetchDocument()
        {
            pugi::xml_document *RVal = IncomingDocument;

            HasUnFetchedXML = false;
            IncomingDocument = nullptr;

            return  std::unique_ptr<pugi::xml_document> (RVal);
        }

        void AsyncTCPXMLClient::ClearReadDataStream()
        {
            ReadDataStream->str(string(""));
        }

        void AsyncTCPXMLClient::Reset()
        {
            CurrentConnectionState = ConnectionState::Disconnected;

            ssl_context.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
            tcp_socket.reset( new boost::asio::ip::tcp::socket(*io_service) );
            ssl_socket.reset( new boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>(*tcp_socket, *ssl_context) );

                    
            
            ReadDataStream = &ReadDataStreamNonSSL;
            ReadDataBuffer = ReadDataBufferNonSSL;
            memset(ReadDataBufferSSL, 0, ReadDataBufferSize);
            memset(ReadDataBufferNonSSL, 0, ReadDataBufferSize);
            ClearReadDataStream();
            HasUnFetchedXML = false;
            IncomingDocument = nullptr;

            SSLConnection = false;

            ssl_socket->set_verify_mode(boost::asio::ssl::verify_peer);
            switch(TLSConfig->Mode)
            {
            case TLSVerificationMode::RFC2818_Hostname:
                ssl_socket->set_verify_callback(
                    boost::asio::ssl::rfc2818_verification("host.name"));
                break;
            case TLSVerificationMode::Custom:
            case TLSVerificationMode::None:
                ssl_socket->set_verify_callback(
                    boost::bind(&AsyncTCPXMLClient::VerifyCertificate,
                                this,
                                _1,
                                _2));
                break;
            }


        }
        
        /*
        void AsyncTCPXMLClient::ForkIO()
        {
            if(IOThread == nullptr)
            {
                IOThread.reset(
                            new boost::thread(boost::bind(
                                                  &boost::asio::io_service::run,
                                                  &io_service)));
            }
            else
            {
                std::cout << "I/O already forked, returning." << std::endl;
            }
            io_service.reset();
        }*/

        void AsyncTCPXMLClient::HandleRead(
                boost::asio::ip::tcp::socket *active_socket,
                char * ActiveBuffer,
                const boost::system::error_code& error,
                std::size_t bytes_transferred)
        {
            if (error == boost::system::errc::operation_canceled)
                return;

            if( CurrentConnectionState != ConnectionState::Connected )
                return;

            if( active_socket != tcp_socket.get() )
                return;
            
            ReadDataBuffer = ActiveBuffer;

            if( ReadDataBuffer == ReadDataBufferSSL )
                ReadDataStream = &ReadDataStreamSSL;
            else
                ReadDataStream = &ReadDataStreamNonSSL;

            if(error)
            {
                CurrentConnectionState = ConnectionState::Error;
                SendKeepAliveWhitespaceTimer = nullptr;
                std::cerr << "HandleRead: Got socket error: " << error
                          << " / " << error.message() << std::endl;
                ErrorCallback();
                return;
            }

            if( bytes_transferred > 0 )
            {
                ReadDataStream->write(ReadDataBuffer, bytes_transferred);
                DebugOut(DebugOutputTreshold::Debug).write(ReadDataBuffer, bytes_transferred);
                DebugOut(DebugOutputTreshold::Debug) << flush;
                GotDataCallback();

                if(HasUnFetchedXML)
                    FetchDocument().release();
            }
            AsyncRead();
        }

        void AsyncTCPXMLClient::AsyncRead()
        {
            if(SSLConnection)
            {
                boost::asio::async_read(*ssl_socket, SSLBuffer,
                                        boost::asio::transfer_at_least(1),
                                        boost::bind(&AsyncTCPXMLClient::HandleRead,
                                                    this,
                                                    tcp_socket.get(),
                                                    ReadDataBufferSSL,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
            }
            else
            {
                boost::asio::async_read(*tcp_socket, NonSSLBuffer,
                                        boost::asio::transfer_at_least(1),
                                        boost::bind(&AsyncTCPXMLClient::HandleRead,
                                                    this,
                                                    tcp_socket.get(),
                                                    ReadDataBufferNonSSL,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
            }
        }


        pugi::xml_node AsyncTCPXMLClient::SelectSingleXMLNode(const char* xpath)
        {
            return IncomingDocument->select_single_node(xpath).node();
        }

        pugi::xpath_node_set AsyncTCPXMLClient::SelectXMLNodes(const char* xpath)
        {
            return IncomingDocument->select_nodes(xpath);
        }



        bool AsyncTCPXMLClient::LoadXML()
        {
            if(HasUnFetchedXML)
                return true;

            string jox = ReadDataStream->str();
            istringstream TStream(jox);
            if( IncomingDocument == NULL)
                IncomingDocument = new pugi::xml_document();

            xml_parse_result parseresult =
                    IncomingDocument->load(TStream, parse_full&~parse_eol, encoding_auto);

            if(parseresult.status != xml_parse_status::status_ok)
                return false;

            HasUnFetchedXML = true;

            ClearReadDataStream();

            DebugOut(DebugOutputTreshold::Debug) << std::endl
                << "++++++++++++++++++++++++++++++" << std::endl;
            DebugOut(DebugOutputTreshold::Debug) << jox;
            DebugOut(DebugOutputTreshold::Debug) << std::endl
                << "------------------------------" << std::endl;
            return true;
        }


        void AsyncTCPXMLClient::WriteXMLToSocket(xml_document *Doc)
        {
            stringstream ss;
            Doc->save(ss, "\t", format_no_declaration);

            if(! (DebugOutputTreshold::Debug > DebugTreshold))
                Doc->save(std::cout, "\t", format_no_declaration);
            WriteTextToSocket(ss.str());
        }


        void AsyncTCPXMLClient::WriteTextToSocket(const string &Data)
        {
            if( CurrentConnectionState != ConnectionState::Connected )
                return;
            
            boost::unique_lock<boost::shared_mutex> WriteLock(this->WriteMutex);
            
            boost::system::error_code ec;
            
            if(SSLConnection)
            {
                /*
                boost::asio::async_write(*ssl_socket, boost::asio::buffer(*SharedData),
                                            boost::bind(&AsyncTCPXMLClient::HandleWrite,
                                                        this,
                                                        tcp_socket.get(),
                                                        SharedData,
                                                        boost::asio::placeholders::error));
                                                        */
                    
                std::size_t written = boost::asio::write(*ssl_socket,boost::asio::buffer(Data),ec);
                if( written != Data.size() || ec != boost::system::errc::success)
                {
                    SendKeepAliveWhitespaceTimer = nullptr;
                    CurrentConnectionState = ConnectionState::Error;
                    std::cerr << "WriteTextToSocket: Got ssl socket error: "
                              << ec << " / " << ec.message() << std::endl;
                    
                    
                    ErrorCallback();
                }
                
                
            }
            else
            {
                /*
                std::shared_ptr<std::string> SharedData = std::make_shared<std::string>(Data);
                boost::asio::async_write(*tcp_socket, boost::asio::buffer(*SharedData),
                                            boost::bind(&AsyncTCPXMLClient::HandleWrite,
                                                        this,
                                                        tcp_socket.get(),
                                                        SharedData,
                                                        boost::asio::placeholders::error));
                                                        */
                
                std::size_t written = boost::asio::write(*tcp_socket,boost::asio::buffer(Data),ec);
                if( written != Data.size() || ec != boost::system::errc::success)
                {
                    SendKeepAliveWhitespaceTimer = nullptr;
                    CurrentConnectionState = ConnectionState::Error;
                    std::cerr << "WriteTextToSocket: Got tcp socket error: "
                              << ec << " / " << ec.message() << std::endl;
                    
                    
                    ErrorCallback();
                }                
            }            
            
            LastWrite = boost::posix_time::microsec_clock::local_time();

            DebugOut(DebugOutputTreshold::Debug) << "Write text to socket:" <<
                std::endl << Data << std::endl;
        }

        bool AsyncTCPXMLClient::EnsureTCPKeepAlive()
        {
            // http://tldp.org/HOWTO/html_single/TCP-Keepalive-HOWTO/
            int optval;
            socklen_t optlen = sizeof(optval);
            if(getsockopt(tcp_socket->lowest_layer().native(), SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0)
            {
                std::cerr << "Unable to getsockopt for keepalive" << std::endl;
                return false;
            }
            if(optval == 1)
                return true;

            /* Set the option active */
            optval = 1;
            optlen = sizeof(optval);
            if(setsockopt(tcp_socket->lowest_layer().native(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
                std::cerr << "Unable to setsockopt for keepalive" << std::endl;
                return false;
            }
            /* Check the status again */
            if(getsockopt(tcp_socket->lowest_layer().native(), SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
                std::cerr << "Unable to very getsockopt for keepalive" << std::endl;
                return false;
            }

            DebugOut(DebugOutputTreshold::Debug)
                << "Set TCP Keep alive on native socket" << std::endl;

            return true;
        }

        void AsyncTCPXMLClient::SendKeepAliveWhitespace()
        {
            if( SendKeepAliveWhitespaceTimer == nullptr )
                return;

            {
                boost::shared_lock<boost::shared_mutex> ReadLock(this->WriteMutex);
                if( LastWrite >
                        (boost::posix_time::microsec_clock::local_time() - boost::posix_time::seconds(SendKeepAliveWhiteSpaceTimeeoutSeconds) )
                   )
                    return;
            }
            WriteTextToSocket(SendKeepAliveWhiteSpaceDataToSend);
            SendKeepAliveWhitespaceTimer->expires_from_now (
                        boost::posix_time::seconds(SendKeepAliveWhiteSpaceTimeeoutSeconds) );
            SendKeepAliveWhitespaceTimer->async_wait(
                        boost::bind(&AsyncTCPXMLClient::SendKeepAliveWhitespace,
                                    this)
                        );
        }

        void AsyncTCPXMLClient::SetKeepAliveByWhiteSpace(const std::string &DataToSend,
                                                         int TimeoutSeconds)
        {            
            SendKeepAliveWhiteSpaceDataToSend = DataToSend;
            SendKeepAliveWhiteSpaceTimeeoutSeconds = TimeoutSeconds;

            SendKeepAliveWhitespaceTimer.reset (
                        new boost::asio::deadline_timer (
                            *io_service, boost::posix_time::seconds(
                                SendKeepAliveWhiteSpaceTimeeoutSeconds) )
                        );

            SendKeepAliveWhitespace();
        }

        bool AsyncTCPXMLClient::ConnectSocket()
        {
            Reset();
            SendKeepAliveWhitespaceTimer = nullptr;
            ReadDataBuffer = ReadDataBufferNonSSL;
            ReadDataStream = &ReadDataStreamNonSSL;

            tcp::resolver resolver(*io_service);
            std::string SPortnumber;
            SPortnumber = boost::lexical_cast<string>( Portnumber);
            tcp::resolver::query query(Hostname, SPortnumber);
            boost::system::error_code io_error = boost::asio::error::host_not_found;
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            boost::asio::connect(tcp_socket->lowest_layer(), endpoint_iterator, io_error);

            if (io_error)
            {
                CurrentConnectionState = ConnectionState::Error;
                std::cerr << "Failed to connect with error: "
                          << io_error << " / " << io_error.message() << std::endl;
                return false;
            }
            else
            {
                DebugOut(DebugOutputTreshold::Debug)
                    << "Succesfully connected socket" << std::endl;
            }

            AsyncTCPXMLClient::EnsureTCPKeepAlive();

            CurrentConnectionState = ConnectionState::Connected;
            return true;
        }

        bool AsyncTCPXMLClient::VerifyCertificate(bool preverified,
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

            if( TLSConfig->Mode == TLSVerificationMode::None)
                return true;

            return TLSConfig->VerifyCertificate(preverified, ctx);
        }

        void AsyncTCPXMLClient::HandleWrite(boost::asio::ip::tcp::socket *active_socket,
                                            std::shared_ptr<std::string> Data,
                                            const boost::system::error_code &error)
        {            
            if( CurrentConnectionState != ConnectionState::Connected )
                return;

            if (error == boost::system::errc::operation_canceled)
                return;

            if(active_socket != tcp_socket.get())
                return;                                    
            
            if(error)
            {                
                SendKeepAliveWhitespaceTimer = nullptr;
                CurrentConnectionState = ConnectionState::Error;
                std::cerr << "Handlewrite: Got socket error: "
                          << error << " / " << error.message() << std::endl;
                
                
                ErrorCallback();

                return;
            }
                       
            //DebugOut(DebugOutputTreshold::Debug) << "Got ack for " << *Data << std::endl;
        }

        bool AsyncTCPXMLClient::ConnectTLSSocket()
        {
            this->CurrentConnectionState = ConnectionState::Upgrading;

            //tcp_socket->cancel();

            //tcp_socket->set_option(tcp::no_delay(true));

            ReadDataBuffer = ReadDataBufferSSL;
            ReadDataStream = &ReadDataStreamSSL;

            SSLConnection = true;
            boost::system::error_code error;

            DebugOut(DebugOutputTreshold::Debug) << "Calling handshake" << std::endl;


            ssl_socket->handshake(boost::asio::ssl::stream_base::client, error);

            if (error)
            {
                std::cerr << "Failed to handshake with error: "
                          << error << " / " << error.message() << std::endl;
                CurrentConnectionState = ConnectionState::Error;
                return false;
            }
            else
            {
                DebugOut(DebugOutputTreshold::Debug)
                    << "Succesfully upgraded to TLS!" << std::endl;
            }

            this->CurrentConnectionState = ConnectionState::Connected;
            return true;
        }
    }
}
