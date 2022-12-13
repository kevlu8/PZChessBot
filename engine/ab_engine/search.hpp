#pragma once

#include "eval.hpp"
#include <unordered_map>

#include <iostream>

/**
 * @brief Calculate the best move for the current position as well as an evaluation of the position.
 *
 * @param position The current position.
 * @param depth The depth to search to.
 * @param metadata The metadata for the current position.
 * @param prev The previous move.
 * @param turn The side to move.
 * @return The best move, in UCI format
 */

const std::string &ab_search(const char *, int, const char *, std::string, const bool) noexcept;
