cmake_minimum_required(VERSION 3.10)
project(LogWatcher)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
 
include_directories(${CMAKE_SOURCE_DIR}/include)
 
set(SOURCES
    src/main.cpp
    src/log_watcher.cpp
    src/web_socket_server.cpp
)
 
add_executable(logwatcher ${SOURCES})
 
target_link_libraries(logwatcher
    PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
)
 
file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})