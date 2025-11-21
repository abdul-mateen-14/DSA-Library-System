cmake_minimum_required(VERSION 3.8)
project(LibraryServer)
set(CMAKE_CXX_STANDARD 17)
include_directories(include)
# Add MySQL connector path (adjust to your installation)
include_directories(/usr/include/mysql-cppconn-8)  # Example path
add_executable(library_server src/main.cpp)
# Link MySQL connector
target_link_libraries(library_server mysqlcppconn8)  # Adjust library name
