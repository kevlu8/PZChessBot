cmake_minimum_required(VERSION 3.5)

project(PZChessBot VERSION 0.1.0)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -m64 -mlzcnt -mbmi -mbmi2 -mavx2 -mpopcnt -DUCI")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -Wno-comment -fsanitize=address,undefined")

# Source files
set(SOURCES
    engine/bitboard.cpp 
    engine/eval.cpp 
    engine/movegen.cpp 
    engine/search.cpp 
    engine/ttable.cpp 
    engine/main.cpp
    engine/nnue/network.cpp
)

# Default executable
add_executable(pzchessbot ${SOURCES})

# HCE version (Handcrafted evaluation)
add_executable(pzchessbot_hce EXCLUDE_FROM_ALL ${SOURCES})
target_compile_definitions(pzchessbot_hce PRIVATE -DHCE)

# Custom target that builds the HCE version
add_custom_target(hce
    DEPENDS pzchessbot_hce
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pzchessbot_hce> ${CMAKE_BINARY_DIR}/pzchessbot
	# Delete the HCE executable now (since we copied it)
	COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:pzchessbot_hce>
    COMMENT "Building with handcrafted evaluation (-DHCE)"
)