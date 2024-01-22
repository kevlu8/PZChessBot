#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "moves.hpp"

extern uint64_t nodes;

Move search(Board &, int);

uint64_t perft(Board &, int);
