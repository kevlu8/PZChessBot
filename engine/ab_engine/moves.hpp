#include <string>
#include <cstring>
#include <utility>
#include <vector>

#include <iostream>

/**
 * @brief Get the number of attackers for each square.
 * @param position The current position.
 * @param side The side to get the number of attackers for.
 * @param controlled The array to store the number of attackers for each square.
 * @param material Whether to consider material value or not.
 */
void controlled_squares(const char *, const bool, char *, const bool);

/**
 * @brief Get a vector of all legal moves for a given position.
 * @param position The current position.
 * @param prev The previous move.
 * @param metadata The metadata for the current position.
 * @return A vector of strings, where each string is a legal move
 */
std::vector<std::string> find_legal_moves(const char *, const std::string, const char *);

/**
 * @brief Check to see if the king is mated
 * @param position The current position.
 * @param king The location of the king.
 * @param control The array of number of attackers for each square.
 * @return True if the king is mated, false otherwise.
 */
bool check_mate(const char *, const int, const char *);

/**
 * @brief Check to see if the king is in check
 * @param position The current position.
 * @param turn The side to check for.
 * @param control The array of number of attackers for each square.
 * @return True if the king is in check, false otherwise.
 */
bool is_check(const char *, const bool, const char *);

/**
 * @brief Make a move on the board.
 * 
 * @param move The move to make.
 * @param prev The previous move.
 * @param newboard The new board after the move is made.
 * @param newmeta The new metadata after the move is made.
 */
void make_move(const std::string, const std::string, char *, char *);
