#include "bitboard.hpp"
#include <queue>

std::pair<int, uint16_t> __recurse(Board &, const int, int, int);

uint16_t ab_search(Board &, const int);
