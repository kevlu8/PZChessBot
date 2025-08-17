#pragma once

#include "includes.hpp"
#include "bitboard.hpp"

std::string score_to_string(Value score);

bool is_capture(Move &m, Board &board);