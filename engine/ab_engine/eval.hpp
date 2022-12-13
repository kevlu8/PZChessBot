#pragma once

#include "moves.hpp"
#include <cmath>
#include <string.h>
#include <string>

/**
 * @brief Evaluate the current position
 *
 * @param board The current position in a const char array
 * @param metadata The metadata of the current position in a const char array
 * @param prev The previous move
 * @param w_control
 * @param b_control
 * @param w_king_pos
 * @param b_king_pos
 * @return Eval of current position in centipawns from White's perspective
 */
int eval(const char *, const char *, const std::string, const char *, const char *, int &, int &) noexcept;
