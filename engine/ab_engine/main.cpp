#include "../fen.hpp"
#include "eval.hpp"
#include "search.hpp"
#include <iomanip>
#include <iostream>

int main() {
	char board[64];
	char meta[3];
	char temp[2];
	std::string fen;
	std::getline(std::cin, fen);

	parse_fen(fen, board, meta, temp);
	// std::cout << "board: " << '\n';
	// for (int i = 0; i < 64; i++) {
	// 	std::cout << std::setw(3) << (int)board[(7 - i / 8) * 8 + i % 8];
	// 	if (i % 8 == 7) {
	// 		std::cout << '\n';
	// 	}
	// }
	// std::cout << std::endl;
	// // std::cout << "turn: " << (int)meta[0] << '\n';

	// // std::vector<std::string> moves = find_legal_moves(board, "0000", meta);
	// // for (std::string s : moves) {
	// // 	std::cout << s << '\n';
	// // }
	// // std::cout << eval(board, meta, temp, "0000") << '\n';
	// std::string move = "128390";
	// while (move != "") {
	// 	std::getline(std::cin, move);
	// 	make_move(move, "0000", board, meta);
	// 	std::cout << "board: " << '\n';
	// 	for (int i = 0; i < 64; i++) {
	// 		std::cout << std::setw(3) << (int)board[(7 - i / 8) * 8 + i % 8];
	// 		if (i % 8 == 7) {
	// 			std::cout << '\n';
	// 		}
	// 	}
	// 	serialize_fen(board, meta, temp, fen);
	// 	std::cout << fen << std::endl;
	// }
	// int depth;
	// std::cin >> depth;
	// std::vector<std::pair<std::string, int>> best = ab_search(board, depth, meta, "0000", meta[0]);
	// for (auto move : best) {
	// 	std::cout << move.first << ' ' << move.second << std::endl;
	// }
	std::cout << eval(board, meta, "0000") << std::endl;
	return 0;
}
