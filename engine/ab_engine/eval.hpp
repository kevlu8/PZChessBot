#include "moves.hpp"
#include <string.h>
#include <string>
#include <cmath>

#include <iomanip>

/**
 * @brief Evaluate the current position
 *
 * @param board The current position in a const char array
 * @param metadata The metadata of the current position in a const char array
 * @param prev The previous move
 * @return Eval of current position in centipawns from White's perspective
 */
int eval(const char *, const char *, const std::string);
