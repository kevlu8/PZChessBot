#include "../fen.hpp"
#include "eval.hpp"
#include <iomanip>
#include <iostream>

int main() {
	char board[64];
	char meta[3];
	char temp[2];
	std::string fen;
	std::cin >> fen;

	parse_fen(fen, board, meta, temp);

	std::vector<std::string> moves = find_legal_moves(board, "0000", 0, meta);
	for (std::string s : moves) {
		std::cout << s << '\n';
	}
	// std::cout << eval(board, meta, temp, "0000") << '\n';
	return 0;
}
