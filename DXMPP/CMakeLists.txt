
project (DXMPP CXX)

add_library(dxmpp
    Connection.cpp
    Network/AsyncTCPXMLClient.cpp

    SASL/SaslChallengeParser.cpp
    SASL/SASLMechanism.cpp
    SASL/SASLMechanism_PLAIN.cpp
    SASL/SASLMechanism_SCRAM_SHA1.cpp
    SASL/SASLMechanism_DIGEST_MD5.cpp

    Roster.cpp

# pure headers added for IDE project files
    JID.hpp
    TLSVerification.hpp
    Stanza.hpp
    StanzaCallback.hpp
    ConnectionCallback.hpp
    IEventHandler.hpp
    Debug/DebugOutputTreshold.hpp

)

if(WIN32)
  target_link_libraries (dxmpp
      pugixml 
      ${Boost_THREAD_LIBRARY}
      ${Boost_SYSTEM_LIBRARY}
      libeay32
      ssleay32
      cryptopp
      libexpat
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  message("Detected FreeBSD")
    target_link_libraries (dxmpp
      pugixml 
      boost_thread 
      boost_system 
      cryptopp
      ssl 
      crypto 
      cryptopp       
      pthread 
      expat
  )
else()
    target_link_libraries (dxmpp
      pugixml 
      ${Boost_THREAD_LIBRARY}
      ${Boost_SYSTEM_LIBRARY}
      cryptopp
      ssl 
      crypto 
      cryptopp       
      pthread 
      dl 
      expat
  )

endif()
