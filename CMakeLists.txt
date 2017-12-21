cmake_minimum_required(VERSION 3.6)
project(CodingChallange)

#set(SOURCE_FILES main.cpp player.h networkReader.h tests/test.cpp)
set(SOURCE_FILES main.cpp player.h networkReader.h )
add_executable(CodingChallange ${SOURCE_FILES})

#link_directories(/home/edaravig/Downloads/googletest-master/googlemock/build/ home/edaravig/Downloads/googletest-master/googletest/build)
#include_directories(/home/edaravig/Downloads/googletest-master/googletest/include/ /home/edaravig/Downloads/googletest-master/googlemock/include/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -march=native")

file(COPY ${PROJECT_SOURCE_DIR}/audio1_s16le_mono_48k.raw DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/audio2_s16le_mono_48k.raw DESTINATION ${CMAKE_BINARY_DIR})
