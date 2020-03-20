cmake_minimum_required(VERSION 3.7)

project(ftpserver)

option(ENABLE_TEST "enable ftpserver test" ON)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
if (DEFINED PKF_CONFIG_FOUND AND NOT DEFINED ACE_INCLUDE_DIRS)
    pkg_check_modules(ACE REQUIRED ACE>=6.5.0)
    list(APPEND FTPSERVER_INCLUDE_DIRS ${ACE_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LINK_LIBRARIES PkgConfig::ACE)
    list(APPEND FTPSERVER_LIBRARY_DIRS ${ACE_LIBRARY_DIRS})
else ()
    if (NOT $ENV{ACE_LIBRARIES})
        message(FATAL_ERROR "cannot found package libACE")
    endif ()
    list(APPEND FTPSERVER_INCLUDE_DIRS ${ACE_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LINK_LIBRARIES ACE)
    list(APPEND FTPSERVER_LIBRARY_DIRS ${ACE_LIBRARY_DIRS})
endif ()

if (DEFINED PKF_CONFIG_FOUND AND NOT DEFINED CXXOPTS_INCLUDE_DIRS)
    pkg_check_modules(cxxopts REQUIRED cxxopts>=1.0)
    list(APPEND FTPSERVER_INCLUDE_DIRS ${cxxopts_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LINK_LIBRARIES PkgConfig::cxxopts)
    list(APPEND FTPSERVER_LIBRARY_DIRS ${cxxopts_LIBRARY_DIRS})
else ()
    if (NOT $ENV{CXXOPTS_LIBRARIES})
        message(FATAL_ERROR "cannot found package cxxopts")
    endif ()
    list(APPEND FTPSERVER_INCLUDE_DIRS ${CXXOPTS_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LIBRARY_DIRS ${CXXOPTS_LIBRARY_DIRS})
endif ()

if (DEFINED PKG_CONFIG_FOUND AND NOT DEFINED CWALK_INCLUDE_DIRS)
    pkg_check_modules(cwalk REQUIRED cwalk>=1.0)
    list(APPEND FTPSERVER_INCLUDE_DIRS ${cwalk_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LINK_LIBRARIES PkgConfig::cwalk)
    list(APPEND FTPSERVER_LIBRARY_DIRS ${cwalk_LIBRARY_DIRS})
else ()
    if (NOT $ENV{CWALK_LIBRARIES})
        message(FATAL_ERROR "cannot found package cwalk")
    endif ()
    list(APPEND FTPSERVER_INCLUDE_DIRS ${CWALK_INCLUDE_DIRS})
    list(APPEND FTPSERVER_LINK_LIBRARIES cwalk)
    list(APPEND FTPSERVER_LIBRARY_DIRS ${CWALK_LIBRARY_DIRS})
endif ()

list(APPEND FTPSERVER_SOURCE_FILES
        ./src/FTPAcceptHandle.cpp
        ./src/FTPOperator.cpp
        ./src/FTPServer.cpp
        ./src/FTPSession.cpp
        ./src/FTPUser.cpp
        ./src/FTPVirtualUser.cpp
        ./src/io/FTPEntity.cpp
        ./src/tls/TLS_Layout.cpp
        ./src/tls/TLS_Record.cpp
        ./src/tls/handshake/TLS_HandShake.cpp
        ./src/tls/handshake/TLS_HelloRequest.cpp
        ./src/ipc/FTPRequest.cpp
        ./src/ipc/FTPSchedule.cpp
        ./main.cpp)

list(APPEND FTPSERVER_LINK_LIBRARIES crypt)

message(STATUS "source_files= ${FTPSERVER_SOURCE_FILES}")
message(STATUS "include_dir = ${FTPSERVER_INCLUDE_DIRS}")
message(STATUS "library_dir = ${FTPSERVER_LIBRARY_DIRS}")
message(STATUS "link_librarys = ${FTPSERVER_LINK_LIBRARIES}")

add_executable(ftpserver ${FTPSERVER_SOURCE_FILES} main.cpp)
target_include_directories(ftpserver PUBLIC ${FTPSERVER_INCLUDE_DIRS})
target_link_directories(ftpserver PUBLIC ${FTPSERVER_LIBRARY_DIRS})
target_link_libraries(ftpserver PUBLIC ${FTPSERVER_LINK_LIBRARIES})
if (ENABLE_TEST)
    add_subdirectory(test)
endif ()

