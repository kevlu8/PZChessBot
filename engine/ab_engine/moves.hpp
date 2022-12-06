#include <string>
#include <utility>
#include <vector>

/**
 * @brief Get a vector of all legal moves for a given position.
 * @param position The current position.
 * @param prev The previous move.
 * @param turn The current turn.
 * @param metadata The metadata for the current position.
 * @return A vector of strings, where each string is a legal move
 */
std::vector<std::string> find_legal_moves(const char *, const std::string, const bool, const char *);

/**
 * @brief Calculate the best move for the current position as well as an evaluation of the position.
 *
 * @param position The current position.
 * @param depth The depth to search to.
 * @return The first element is the best move, the second is the evaluation of the position.
 */

std::pair<std::string, int> ab_search(char *, int);
