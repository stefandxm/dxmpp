
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

target_link_libraries (dxmpp
    pugixml boost_thread boost_system ssl crypto cryptopp pthread dl expat
)
