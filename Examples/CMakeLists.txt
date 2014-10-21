project (DXMPPTests CXX)

add_executable(echobot
	EchoBot.cpp 
)

target_link_libraries(echobot
	dxmpp
)
