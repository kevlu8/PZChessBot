#include "bitboard.hpp"
#include <queue>

#ifdef PERFT
static const constexpr bool NOPRUNE = true;
static const constexpr bool QUIESCENCE = false;
#else
static const constexpr bool NOPRUNE = false;
#ifndef NOQUIESCENCE
static const constexpr bool QUIESCENCE = true;
#else
static const constexpr bool QUIESCENCE = false;
#endif
#endif

std::pair<int, uint16_t> __recurse(Board &, const int, int, int);

std::pair<int, uint16_t> ab_search(Board &, const int);

/**
 * @brief Returns the number of nodes in the last search
 *
 * @return The number of nodes in the last search
 */
const unsigned long long nodes();
