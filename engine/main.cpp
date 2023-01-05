#include "bitboard.hpp"
#include "search.hpp"

#ifdef SEARCH_RESULT
static const constexpr bool RESULT = true;
#else
static const constexpr bool RESULT = false;
#endif

int main() {
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
		if (PRINTBOARD) {
			board.print_board();
			std::cout << std::endl;
		}
		if (RESULT) {
			std::pair<int, uint16_t> res = ab_search(board, depth);
			std::cout << "eval: " << res.first << std::endl;
			std::cout << "move: " << stringify_move(res.second) << std::endl;
		} else {
			ab_search(board, depth);
			std::cout << nodes() << std::endl;
		}
	}
	return 0;
}
