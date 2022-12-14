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
	// char w_control[64], b_control[64];
	// int tmp, tmp2;
	// controlled_squares(board, true, w_control, false);
	// controlled_squares(board, false, b_control, false);
	// std::cout << eval(board, w_control, b_control, tmp, tmp2) << std::endl;
	return 0;
}
