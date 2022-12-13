#include "../fen.hpp"
#include "eval.hpp"
#include "search.hpp"
#include <iostream>

int main() {
	char board[64];
	char meta[3];
	char temp[2];
	std::string fen;
	std::getline(std::cin, fen);

	parse_fen(fen, board, meta, temp);

	int depth;
	std::cin >> depth;
	std::string best = ab_search(board, depth, meta, "0000", meta[0]);
	std::cout << best << std::endl;
	// std::cout << eval(board, meta, "0000") << std::endl;
	return 0;
}
