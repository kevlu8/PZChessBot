#include "bitboard.hpp"
#include "search.hpp"

int main() {
	// search
	std::string fen;
	std::string tmp;
	int depth;
	std::getline(std::cin, fen);
	std::getline(std::cin, tmp);
	depth = std::stoi(tmp);
	Board board(fen);
	// board.print_board();
	// std::cout << std::endl;
	// std::cout << serialize_move(ab_search(board, depth)) << std::endl;
	ab_search(board, depth);
	return 0;
}
