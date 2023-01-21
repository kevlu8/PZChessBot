#include "bitboard.hpp"
#include "search.hpp"

int main() {
	// std::string fen, move;
	// std::getline(std::cin, fen);
	// Board board(fen);
	// while (true) {
	// 	board.print_board();
	// 	std::cin >> move;
	// 	if (move == "quit")
	// 		break;
	// 	board.make_move(parse_move(board, move));
	// 	board.print_board();
	// 	uint16_t thing = ab_search(board, 7).second;
	// 	board.make_move(thing);
	// 	std::cout << stringify_move(thing) << std::endl;
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
		std::cout << "eval: " << res.first << std::endl;
		std::cout << "move: " << stringify_move(res.second) << std::endl;
		std::cout << "nodes: " << nodes() << std::endl << std::endl;
	}
	return 0;
}
