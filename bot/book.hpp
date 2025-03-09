#pragma once

#include "../engine/bitboard.hpp"
#include "api.hpp"
#include <cpr/cpr.h>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

void init_book();

std::pair<Move, bool> book_move(std::vector<std::string> &moves, Board &board);