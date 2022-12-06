#include "moves.hpp"
#include <string>

/**
 * @brief Evaluate the current position
 *
 * @param board The current position in a const char array
 * @param metadata The metadata of the current position in a const char array
 * @param extra_metadata The extra metadata of the current position in a const char array
 * @param prev The previous move
 * @return Eval of current position in centipawns from White's perspective
 */
int eval(const char *, const char *, const char *, std::string);
