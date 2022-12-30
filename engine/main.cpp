#include "bitboard.hpp"
#include "search.hpp"

int main() {
	// // play moves
	// std::string fen;
	// std::string tmp;
	// // int depth;
	// std::getline(std::cin, fen);
	// // std::getline(std::cin, tmp);
	// // depth = std::stoi(tmp);
	// Board board(fen);
	// board.print_board();
	// // std::cout << ab_search(board, depth) << std::endl;
	// while (true) {
	// 	std::getline(std::cin, tmp);
	// 	if (tmp == "u")
	// 		board.unmake_move();
	// 	else
	// 		board.make_move(std::stoi(tmp));
	// 	board.print_board();
	// }

	// search
	std::string fen;
	std::string tmp;
	int depth;
	std::getline(std::cin, fen);
	std::getline(std::cin, tmp);
	depth = std::stoi(tmp);
	Board board(fen);
	board.print_board();
	std::cout << std::endl;
	std::cout << serialize_move(ab_search(board, depth)) << std::endl;
	return 0;
}
