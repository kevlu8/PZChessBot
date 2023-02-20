#include "bitboard.hpp"
#include <queue>
#include <tuple>
#include <unordered_map>

#ifdef PERFT
#define NOPRUNE true
#define QUIESCENCE false
#else
#define NOPRUNE false
#ifndef NOQUIESCENCE
#define QUIESCENCE true
#else
#define QUIESCENCE false
#endif
#endif

#ifdef PRINTLINE
std::pair<int, uint16_t> __recurse(Board &, const int, int, int, std::deque<std::pair<int, uint16_t>> **);
#else
std::pair<int, uint16_t> __recurse(Board &, const int, int, int);
#endif

std::pair<int, uint16_t> ab_search(Board &, const int /*, std::deque<std::pair<int, uint16_t>> ***/);

/**
 * @brief Returns the number of nodes in the last search
 *
 * @return The number of nodes in the last search
 */
const unsigned long long nodes();
