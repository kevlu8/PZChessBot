#pragma once

#include "bitboard.hpp"
#include "nnue/network.hpp"
#include "includes.hpp"

void init_network();

Value eval(Board &board);
