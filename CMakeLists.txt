cmake_minimum_required(VERSION 3.4.0 FATAL_ERROR)

project(server)
find_package(boost 1.59.0 REQUIRED)
find_package(lua 5.3.2 REQUIRED)
add_executable(server main.cpp metadata.cpp metalua.cpp metastate.cpp bot.cpp)
target_link_libraries(server boost_system lua)
set_property(TARGET server PROPERTY CXX_STANDARD 14)


# Until I figure out how to build up oauth or import an implementation
# for the twitter bits I'm keeping this on the side for experimenting
project(oauth)
add_executable(oauth oauth.cpp)
set_property(TARGET oauth PROPERTY CXX_STANDARD 14)
