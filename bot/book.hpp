#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cpr/cpr.h>
#include "../engine/bitboard.hpp"
#include "json.hpp"

using json = nlohmann::json;

void init_book();

std::pair<Move, bool> book_move(std::vector<std::string> &moves, Board &board);