#include "search.hpp"

std::pair<int, uint16_t> __recurse(Board b, int depth, const int maxdepth) { std::set<std::pair<int, uint16_t>> orderedmoves; }

uint16_t ab_search(Board b, const int maxdepth) { return __recurse(b, 0, maxdepth).second; }
