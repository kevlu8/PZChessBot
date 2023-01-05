#include "bitboard.hpp"
#include <queue>

#ifdef DEBUG_PERFT
static const constexpr bool PRINT = true;
#ifdef DEBUG_MOVES
static const constexpr bool PRINTBOARD = true;
#else
static const constexpr bool PRINTBOARD = false;
#endif
#else
static const constexpr bool PRINT = false;
static const constexpr bool PRINTBOARD = false;
#endif

std::pair<int, uint16_t> __recurse(Board &, const int, int, int);

std::pair<int, uint16_t> ab_search(Board &, const int);
