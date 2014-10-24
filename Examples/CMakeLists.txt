project (DXMPPTests CXX)

add_executable(echobot
	EchoBot.cpp 
)

add_executable(rosterbot
    RosterBot.cpp
)

add_executable(tlsbot
    TLSBot.cpp
)

target_link_libraries(tlsbot
    dxmpp
)
target_link_libraries(echobot
	dxmpp
)

target_link_libraries(rosterbot
    dxmpp
)
