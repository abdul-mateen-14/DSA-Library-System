cmake_minimum_required(VERSION 3.10)
project(LibraryServer)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(library_server src/main.cpp)

find_package(Boost REQUIRED COMPONENTS system thread)

target_link_libraries(library_server
    sqlite3
    pthread
    boost_system
    boost_thread
    ssl
    crypto
)
