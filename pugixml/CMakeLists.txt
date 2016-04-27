
project (pugixml CXX)

add_library(pugixml
# Please note this has to be downloaded and added externally to DXMPP
# PUGIXML is available in MIT license
# from http://pugixml.org and only requires an unpack into
# this folder.

	pugixml.cpp
)

install (TARGETS pugixml DESTINATION lib)
install (DIRECTORY "${PROJECT_SOURCE_DIR}" DESTINATION "include/pugixml" FILES_MATCHING PATTERN "*.h*")
