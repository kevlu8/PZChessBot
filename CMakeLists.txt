cmake_minimum_required(VERSION 3.5)

project(PZChessBot)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -m64 -mlzcnt -mbmi -mbmi2 -mavx2 -mpopcnt")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -pedantic -Wno-comment -fsanitize=address,undefined")

add_executable(pzchessbot engine/bitboard.cpp engine/eval.cpp engine/movegen.cpp engine/search.cpp engine/ttable.cpp engine/main.cpp)

target_link_libraries(pzchessbot curl pthread)
