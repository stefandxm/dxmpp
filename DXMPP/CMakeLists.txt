
project (DXMPP CXX)

add_library(dxmpp 
	Connection.cpp	
    Network/AsyncTCPXMLClient.cpp

    SASL/SaslChallengeParser.cpp
    SASL/SASLMechanism.cpp
    SASL/SASLMechanism_PLAIN.cpp
    SASL/SASLMechanism_SCRAM_SHA1.cpp
    SASL/SASLMechanism_DIGEST_MD5.cpp

# pure headers added for IDE project files
	Stanza.hpp
	StanzaCallback.hpp
	ConnectionCallback.hpp
    Debug/DebugOutputTreshold.hpp


# Please note this has to be downloaded and added externally to DXMPP
# PUGIXML is available in MIT license
# from http://pugixml.org and only requires an unpack into
# pugixml folder.
	pugixml/pugixml.cpp
)

target_link_libraries (dxmpp
	boost_thread boost_system crypto ssl cryptopp pthread
)
