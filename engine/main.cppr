#include "fen.hpp"
#include <iostream>
#include <string>

int main() {
	std::string fen;
	std::getline(std::cin, fen);
	char board[64];
	char metadata[4];
	char extra_metadata[2];
	parse_fen(fen, board, metadata, extra_metadata);
	serialize_fen(board, metadata, extra_metadata, fen);
	std::cout << fen << std::endl;
	return 0;
}
