cmake_minimum_required(VERSION 3.4.0 FATAL_ERROR)

project(metadata)
find_package(boost 1.59.0 REQUIRED)
add_executable(metadata metadata.cpp)
target_link_libraries(metadata boost_system)
set_property(TARGET metadata PROPERTY CXX_STANDARD 14)

project(oauth)
add_executable(oauth oauth.cpp)
set_property(TARGET oauth PROPERTY CXX_STANDARD 14)
