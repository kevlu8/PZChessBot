#include "bitboard.hpp"
#include "search.hpp"

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
		ab_search(board, depth);
	}
	return 0;
}
