#pragma once

#include "board.hpp"

#include <memory.h>
#include <string>

const char fen_to_char[17] = {
	3, -1, -1, -1, -1, -1, -1, -1, -1, 6, -1, -1, 2, -1, 1, 5, 4,
};

const char char_to_fen[7] = {
	'.', 'P', 'N', 'B', 'R', 'Q', 'K',
};

/**
 * @brief Parses a FEN string into a board and metadata.
 *
 * @param fen The FEN string to parse.
 * @param metadata The metadata to write to.
 * @return The board.
 */
Board &parse_fen(const std::string, char *);
/**
 * @brief Serializes a board and metadata into a FEN string.
 *
 * @param board The board to read from.
 * @param metadata The metadata to read from.
 * @param extra_metadata Extra metadata to append to the FEN string.
 * @param fen The FEN string to write to.
 */
void serialize_fen(const char *, const char *, const char *, std::string &);
