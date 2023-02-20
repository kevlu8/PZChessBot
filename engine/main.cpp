#include "bitboard.hpp"
#include "search.hpp"

int main() {
	// std::string fen, move;
	// std::getline(std::cin, fen);
	// Board board(fen);
	// while (true) {
	// 	board.print_board();
	// 	std::cout << board.hash << ' ' << (bool)board.meta[0] << ' ' << (int)board.meta[1] << ' ' << (int)board.meta[3] << std::endl;
	// 	std::cin >> move;
	// 	if (move == "quit")
	// 		break;
	// 	if (move == "u")
	// 		board.unmake_move();
	// 	else
	// 		board.make_move(parse_move(board, move));
	// }
	// search
	std::string fen;
	std::string tmp;
	int depth;
	while (true) {
		std::getline(std::cin, fen);
		std::getline(std::cin, tmp);
		if (fen == "quit" || tmp == "quit")
			break;
		depth = std::stoi(tmp);
		Board board(fen);
		std::pair<int, uint16_t> res = ab_search(board, depth);
		std::cout << "move: " << stringify_move(res.second) << std::endl;
		std::cout << "eval: " << res.first << std::endl;
		std::cout << "nodes: " << nodes() << std::endl << std::endl;
	}
	return 0;
}
