cmake_minimum_required(VERSION 3.5)

project(PZChessBot)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOQUIESCENCE -DPRINTLINE -std=c++17 -m64 -mlzcnt -mbmi -mbmi2 -mavx2 -mpopcnt")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -pedantic -Wno-comment")

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG dd967cb48ea6bcbad9f1da5ada0db8ac0d532c06) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

add_executable(pzchessbot bot/api.cpp bot/main.cpp engine/bitboard.cpp engine/eval.cpp engine/movegen.cpp engine/search.cpp)

target_link_libraries(pzchessbot cpr)

target_link_libraries(pzchessbot pthread)
