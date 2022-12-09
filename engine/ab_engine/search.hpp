#include "eval.hpp"

/**
 * @brief Calculate the best move for the current position as well as an evaluation of the position.
 *
 * @param position The current position.
 * @param depth The depth to search to.
 * @param metadata The metadata for the current position.
 * @param prev The previous move.
 * @param turn The side to move.
 * @return A vector of pairs: the first element is the best move, the second is the evaluation of the position.
 */

std::vector<std::pair<std::string, int>> ab_search(const char *, int, const char *, std::string, const bool) noexcept;
