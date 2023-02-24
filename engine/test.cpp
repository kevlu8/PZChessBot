#include "bitboard.hpp"
#include "search.hpp"

int main() {
	std::string fen, str;
	std::getline(std::cin, fen);
	Board board(fen);
	board.print_board();
	std::cout << board.zobrist_hash() << ' ' << board.currhash() << std::endl;
	std::pair<int, uint16_t> res;
	res = ab_search(board, 3);
	std::cout << stringify_move(res.second) << std::endl;
	// std::cin >> str;
	// int i = 3;
	// while (1) {
	// 	if (str == "u")
	// 		board.unmake_move();
	// 	else if (str == "q")
	// 		break;
	// 	else
	// 		board.make_move(parse_move(board, str));
	// 	board.print_board();
	// 	std::cout << board.zobrist_hash() << ' ' << board.currhash() << std::endl;
	// 	std::cin >> str;
	// }
	return 0;
}
