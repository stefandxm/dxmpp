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
#include <iostream>
#include <expat.h>

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
            boost::unique_lock<boost::shared_mutex> Lock(IncomingDocumentsMutex);
            if( IncomingDocuments.empty() )
                return nullptr;

            std::unique_ptr<pugi::xml_document> RVal(IncomingDocuments.front());
            IncomingDocuments.pop();

            return RVal;
        }

        void AsyncTCPXMLClient::ClearReadDataStream()
        {
            ReadDataStream->str(string(""));
        }

        void AsyncTCPXMLClient::Reset()
        {
            CurrentConnectionState = ConnectionState::Disconnected;

            ssl_context.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
            if(Certificate.size() > 0)
                ssl_context->use_certificate(Certificate, boost::asio::ssl::context::file_format::pem);
            if(Privatekey.size()>0)
                ssl_context->use_private_key(Privatekey, boost::asio::ssl::context::file_format::pem);
            tcp_socket.reset( new boost::asio::ip::tcp::socket(*io_service) );
            ssl_socket.reset( new boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>(*tcp_socket, *ssl_context) );



            ReadDataStream = &ReadDataStreamNonSSL;
            ReadDataBuffer = ReadDataBufferNonSSL;
            memset(ReadDataBufferSSL, 0, ReadDataBufferSize);
            memset(ReadDataBufferNonSSL, 0, ReadDataBufferSize);
            ClearReadDataStream();

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


            ReadDataBuffer = ActiveBuffer;

            if( ReadDataBuffer == ReadDataBufferSSL )
                ReadDataStream = &ReadDataStreamSSL;
            else
                ReadDataStream = &ReadDataStreamNonSSL;

            if(error)
            {
                std::cerr << "HandleRead: Got socket error: " << error
                          << " / " << error.message() << std::endl;
                CurrentConnectionState = ConnectionState::Error;
                SendKeepAliveWhitespaceTimer = nullptr;
                ErrorCallback();
                return;
            }

            if( bytes_transferred > 0 )
            {
                std::stringstream TStream;
                TStream.write(ReadDataBuffer, bytes_transferred);
                ReadDataStream->str(ReadDataStream->str() + TStream.str());
                DebugOut(DebugOutputTreshold::Debug).write(ReadDataBuffer, bytes_transferred);
                DebugOut(DebugOutputTreshold::Debug) << flush;
                GotDataCallback();
            }
            AsyncRead();
        }

        void AsyncTCPXMLClient::AsyncRead()
        {
            if(SSLConnection)
            {

                SynchronizationStrand.post([&, this]
                {
                    boost::asio::async_read(*ssl_socket, SSLBuffer,
                                            boost::asio::transfer_at_least(1),
                                            SynchronizationStrand.wrap(
                                                boost::bind(&AsyncTCPXMLClient::HandleRead,
                                                            this,
                                                            tcp_socket.get(),
                                                            ReadDataBufferSSL,
                                                            boost::asio::placeholders::error,
                                                            boost::asio::placeholders::bytes_transferred)
                                                ));
                });
            }
            else
            {
                boost::asio::async_read(*tcp_socket, NonSSLBuffer,
                                        boost::asio::transfer_at_least(1),
                                        SynchronizationStrand.wrap(
                                            boost::bind(&AsyncTCPXMLClient::HandleRead,
                                                        this,
                                                        tcp_socket.get(),
                                                        ReadDataBufferNonSSL,
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred)
                                            ));
            }
        }


        class ExpatStructure
        {
        public:
            AsyncTCPXMLClient * Sender;
            XML_Parser Parser;
            string RootTagName;
            long EndPosition;
            int  NrSubRootTagNamesFound;

            ExpatStructure( AsyncTCPXMLClient *Sender, XML_Parser Parser)
                :
                  Sender(Sender),
                  Parser(Parser)
            {
                RootTagName = "";
                EndPosition = 0;
                NrSubRootTagNamesFound = 0;
            }
        };

        void SAXStartElementHandler(void* data, const XML_Char* el, const XML_Char **atts)
        {
            ExpatStructure *ES = static_cast<ExpatStructure*>(data);
            if( ES->EndPosition > 0 )
                return;

            string StrEl(el);
            if(ES->RootTagName.length() == 0)
                ES->RootTagName = StrEl;

            if( ES->RootTagName == StrEl )
                ES->NrSubRootTagNamesFound++;
        }

        void AsyncTCPXMLClient::SignalError()
        {
            CurrentConnectionState = ConnectionState::Error;
            ErrorCallback();
        }

        void SAXEndElementHandler(void* data, const XML_Char* el)
        {
            ExpatStructure *ES = static_cast<ExpatStructure*>(data);
            if( ES->EndPosition > 0 )
                return;

            string StrEl(el);

            if(StrEl == string("</stream:stream>"))
            {
                std::cerr << "Received end of stream" << std::endl;
                ES->Sender->SignalError();
                return;
            }

            if(StrEl != ES->RootTagName )
                return;

            if( (--ES->NrSubRootTagNamesFound) == 0 )
            {
                ES->EndPosition = XML_GetCurrentByteIndex(ES->Parser) + XML_GetCurrentByteCount(ES->Parser);
            }
        }


        bool AsyncTCPXMLClient::InnerLoadXML()
        {
            string jox = ReadDataStream->str();

            if(jox.empty())
                return false;

            string CompleteXML;
            XML_Parser p = XML_ParserCreate(NULL);
            ExpatStructure ExpatData(this, p);

            XML_SetUserData( p, &ExpatData );
            XML_SetElementHandler(p, SAXStartElementHandler, SAXEndElementHandler);

            XML_Parse(p, jox.c_str(), jox.length(), true);
            XML_ParserFree(p);

            if( ExpatData.EndPosition == 0 )
            {
                //std::cerr << "incomplete: " << jox << std::endl;
                return false;
            }

            CompleteXML = jox.substr(0, ExpatData.EndPosition);
            string NewXML = jox.substr(CompleteXML.length());
            //std::cout << "new xml:" << endl << "#" << NewXML <<  "#" << endl;
            ReadDataStream->str(NewXML);

            istringstream TStream(CompleteXML);

            pugi::xml_document *NewDoc = new pugi::xml_document();

            xml_parse_result parseresult =
                    NewDoc->load(TStream, parse_full&~parse_eol, encoding_auto);

            if(parseresult.status != xml_parse_status::status_ok)
            {
                std::cerr << "Failed to parse xml: " << CompleteXML << std::endl;
                delete NewDoc;
                return true;
            }

            boost::unique_lock<boost::shared_mutex> Lock(IncomingDocumentsMutex);
            IncomingDocuments.push(NewDoc);

            DebugOut(DebugOutputTreshold::Debug) << std::endl
                << "++++++++++++++++++++++++++++++" << std::endl;
            DebugOut(DebugOutputTreshold::Debug) << CompleteXML;
            DebugOut(DebugOutputTreshold::Debug) << std::endl
                << "------------------------------" << std::endl;
            return true;
        }

        void AsyncTCPXMLClient::LoadXML()
        {
            if( ReadDataStream->str().empty() )
                return;

            //std::cout << "+++before inner load xmls: "<<std::endl << ReadDataStream->str() << std::endl  << "---before inner load xmls" <<std::endl;

            while(InnerLoadXML())
            {
            }

            //std::cout << "+++after inner load xmls: "<<std::endl << ReadDataStream->str() << std::endl  << "---after inner load xmls" <<std::endl;
        }

        bool AsyncTCPXMLClient::LoadXML(int Iteration)
        {
            if( Iteration == 0 && ReadDataStream->str().empty() )
                return false;

            //std::cout << "+++before inner load xmls: "<<std::endl << ReadDataStream->str() << std::endl  << "---before inner load xmls" <<std::endl;

            /*int NrLoaded = 0;
            while(InnerLoadXML() && (++NrLoaded) < 5)
            {
                NrLoaded++;
                if(NrLoaded > 5)
                    return;
                // Do nothing
            }*/

            return InnerLoadXML();
            //std::cout << "+++after inner load xmls: "<<std::endl << ReadDataStream->str() << std::endl  << "---after inner load xmls" <<std::endl;
        }


        void AsyncTCPXMLClient::WriteXMLToSocket(xml_document *Doc)
        {

            stringstream ss;
            Doc->save(ss, "\t", format_no_declaration);

            if(! (DebugOutputTreshold::Debug > DebugTreshold))
                Doc->save(std::cout, "\t", format_no_declaration);

            //boost::unique_lock<boost::shared_mutex> WriteLock(this->WriteMutex);
            WriteTextToSocket(ss.str());
        }


        void AsyncTCPXMLClient::FlushOutgoingDataUnsafe()
        {
            if(OutgoingData.empty())
                return;

            Flushing = true;

            std::shared_ptr<std::string> SharedData = OutgoingData.front();
            OutgoingData.pop();

            boost::system::error_code ec;
            if(SSLConnection)
            {
                SynchronizationStrand.post([&,SharedData, this]
                {
                    boost::asio::async_write(*ssl_socket, boost::asio::buffer(*SharedData),
                                             SynchronizationStrand.wrap(
                                                boost::bind(&AsyncTCPXMLClient::HandleWrite,
                                                            this,
                                                            tcp_socket.get(),
                                                            SharedData,
                                                            boost::asio::placeholders::error)
                                                 ));
                });

                /*
                std::size_t written = boost::asio::write(*ssl_socket,boost::asio::buffer(Data),ec);
                if( written != Data.size() || ec != boost::system::errc::success)
                {
                    SendKeepAliveWhitespaceTimer = nullptr;
                    CurrentConnectionState = ConnectionState::Error;
                    std::cerr << "WriteTextToSocket: Got ssl socket error: "
                              << ec << " / " << ec.message() << std::endl;


                    ErrorCallback();
                }*/
            }
            else
            {
                boost::asio::async_write(*tcp_socket, boost::asio::buffer(*SharedData),
                                         SynchronizationStrand.wrap(
                                            boost::bind(&AsyncTCPXMLClient::HandleWrite,
                                                        this,
                                                        tcp_socket.get(),
                                                        SharedData,
                                                        boost::asio::placeholders::error)
                                             ));

            /*
                std::size_t written = boost::asio::write(*tcp_socket,boost::asio::buffer(Data),ec);
                if( written != Data.size() || ec != boost::system::errc::success)
                {
                    SendKeepAliveWhitespaceTimer = nullptr;
                    CurrentConnectionState = ConnectionState::Error;
                    std::cerr << "WriteTextToSocket: Got tcp socket error: "
                              << ec << " / " << ec.message() << std::endl;


                    ErrorCallback();
                }*/
            }

            LastWrite = boost::posix_time::microsec_clock::universal_time();

            DebugOut(DebugOutputTreshold::Debug) << "Write text to socket:" <<
                std::endl << "+++" << std::endl << *SharedData << std::endl << " --- " << std::endl;
        }

        void AsyncTCPXMLClient::FlushOutgoingData()
        {
            boost::unique_lock<boost::shared_mutex> WriteLock(OutgoingDataMutex);

            if(Flushing)
                return;

            FlushOutgoingDataUnsafe();
        }


        void AsyncTCPXMLClient::WriteTextToSocket(const string &Data)
        {
            if( CurrentConnectionState != ConnectionState::Connected )
                return;

            {
                boost::unique_lock<boost::shared_mutex> WriteLock(OutgoingDataMutex);
                std::shared_ptr<std::string> SharedData (new std::string(Data));
                OutgoingData.push( SharedData );

                if(!Flushing)
                    FlushOutgoingDataUnsafe();  /* we have lock */
            }


            return;
        }

        bool AsyncTCPXMLClient::EnsureTCPKeepAlive()
        {
#ifndef WIN32
#ifdef BOOSTASIOHASNATIVE
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
#endif // BOOSTASIOHASNATIVE
#endif // !WIN32
            return true;
        }

        void AsyncTCPXMLClient::SendKeepAliveWhitespace()
        {
            if( SendKeepAliveWhitespaceTimer == nullptr )
                return;

            //boost::unique_lock<boost::shared_mutex> WriteLock(this->WriteMutex);
            if( LastWrite >
                    (boost::posix_time::microsec_clock::universal_time() - boost::posix_time::seconds(SendKeepAliveWhiteSpaceTimeeoutSeconds) )
               )
            {
                SendKeepAliveWhitespaceTimer->expires_from_now (
                            boost::posix_time::seconds(SendKeepAliveWhiteSpaceTimeeoutSeconds) );
                SendKeepAliveWhitespaceTimer->async_wait(
                            boost::bind(&AsyncTCPXMLClient::SendKeepAliveWhitespace,
                                        this)
                            );

                return;
            }

            WriteTextToSocket(SendKeepAliveWhiteSpaceDataToSend);
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


            if(error)
            {
                std::cerr << "Handlewrite: Got socket error: "
                          << error << " / " << error.message() << std::endl;

                SendKeepAliveWhitespaceTimer = nullptr;
                CurrentConnectionState = ConnectionState::Error;

                ErrorCallback();

                return;
            }

            {
                boost::unique_lock<boost::shared_mutex> WriteLock(OutgoingDataMutex);
                if(OutgoingData.empty())
                {
                    Flushing = false;
                    return;
                }

                FlushOutgoingDataUnsafe();
            }

            //DebugOut(DebugOutputTreshold::Debug) << "Got ack for " << *Data << std::endl;
        }

        bool AsyncTCPXMLClient::ConnectTLSSocket()
        {
            ReadDataStream->str("");
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
