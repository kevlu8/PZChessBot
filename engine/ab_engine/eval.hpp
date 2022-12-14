#pragma once

#include "moves.hpp"
#include <cmath>
#include <string.h>
#include <string>

/**
 * @brief Quickly evaluate the current position
 *
 * @param board The current position in a const char array
 * @param newboard The new position in a const char array
 * @param control Controlled squares of the player
 * @param opp_control Controlled squares of the opponent
 * @param turn True if it's white's turn, false if it's black's turn
 * @param move The move that was made
 * @return Eval of current position in centipawns from White's perspective
 * 
 */
int heuristic(const char *, const char *, const char *, const char *, const bool, const std::string);

/**
 * @brief Evaluate the current position
 *
 * @param board The current position in a const char array
 * @param w_control Controlled squares of white
 * @param b_control Controlled squares of black
 * @param w_king_pos Variable to store white's king position in
 * @param b_king_pos Variable to store black's king position in
 * @return Eval of current position in centipawns from White's perspective
 */
int eval(const char *, const char *, const char *, int *, int *) noexcept;
