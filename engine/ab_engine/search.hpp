#pragma once

#include "../board.hpp"
#include "zobrist"

#include <set>
#include <unordered_map>
#include <vector>

#include <iostream>

/**
 * @brief Calculate the best move for the current position as well as an evaluation of the position.
 *
 * @param board The current position.
 * @param depth The depth to search to.
 * @return The best move, in UCI format
 */

const std::string ab_search(Board *, int) noexcept;
