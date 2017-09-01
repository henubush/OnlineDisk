cmake_minimum_required(VERSION 2.6)

project(pas CXX)

set(CXX_FLAGS -g -Wall)
set(CMAKE_CXX_COMPILER "g++")
string(REPLACE ";" " " CAMKE_CXX_FLAGS "${CXX_FLAGS}")



set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


add_executable(chunk chunk.cc client.cc client.h)
target_link_libraries(chunk muduo_net muduo_base pthread jsoncpp)



