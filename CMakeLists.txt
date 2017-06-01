cmake_minimum_required(VERSION 2.8)

project(deusexmachinaexmpppframework CXX)


set(CMAKE_VERBOSE_MAKEFILE ON) 

# from http://www.cmake.org/cmake/help/v2.8.8/cmake.html#variable:CMAKE_LANG_COMPILER_ID

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  MESSAGE( ERROR "Intel compiler not supported in our cmake..  Please add" )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  MESSAGE( ERROR "Microsoft compiler not supported in our cmake..  Please add" )
endif()

MESSAGE( STATUS "CMAKE_BINARY_DIR:        ${CMAKE_BINARY_DIR}" )
MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID:   ${CMAKE_CXX_COMPILER_ID}" )
MESSAGE( STATUS "CMAKE_CXX_COMPILER:	  ${CMAKE_CXX_COMPILER}" )
MESSAGE( STATUS "CMAKE_CXX_FLAGS:         ${CMAKE_CXX_FLAGS}" )
MESSAGE( STATUS "EXTRA INCLUDE:           ${EXTRAINCLUDE}")
MESSAGE( STATUS "EXTRA LIB:               ${EXTRALIB}")
MESSAGE( STATUS "BOOST INCLUDE:           ${BOOST_INCLUDEDIR}")
MESSAGE( STATUS "BOOST LIB:               ${BOOST_LIBRARYDIR}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED ON)

#set(BOOST_INCLUDEDIR ${BOOSTINCLUDE})
#set(BOOST_LIBRARYDIR ${BOOSTLIB})

FIND_PACKAGE(Boost REQUIRED thread system date_time)
IF (Boost_FOUND)
    MESSAGE( STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX} " )
    MESSAGE( STATUS "Boost include was found in ${Boost_INCLUDE_DIR} " )
    MESSAGE( STATUS "boostlibs: ${Boost_LIBRARY_DIR}" )
    MESSAGE( STATUS "boost thread    : ${Boost_THREAD_LIBRARY}" )
    MESSAGE( STATUS "boost system    : ${Boost_SYSTEM_LIBRARY}" )
ELSE()
    MESSAGE( ERROR "Could not find Boost" )
ENDIF()

include_directories(${EXTRAINCLUDE} ${CMAKE_INSTALL_PREFIX}/include ${Boost_INCLUDE_DIR} $(PROJECT_SOURCE_DIR))
link_directories(${EXTRALIB} ${DCMAKE_INSTALL_PREFIX}/lib)
add_subdirectory(pugixml)
add_subdirectory(DXMPP)
#add_subdirectory(Examples)
