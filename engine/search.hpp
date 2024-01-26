#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "moves.hpp"

extern uint64_t nodes;

Move search(Board &board, int depth);

uint64_t perft(Board &board, int depth);
