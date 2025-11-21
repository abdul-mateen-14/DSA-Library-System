cmake_minimum_required(VERSION 3.8)
project(LibraryServer)
set(CMAKE_CXX_STANDARD 17)


# include local headers
include_directories(include)


find_package(SQLite3 REQUIRED)


add_executable(library_server src/main.cpp)


# Link SQLite3 and pthread
target_link_libraries(library_server SQLite::SQLite3 pthread)
