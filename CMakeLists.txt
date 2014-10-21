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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories($(PROJECT_SOURCE_DIR))
add_subdirectory(DXMPP)
add_subdirectory(Tests)
