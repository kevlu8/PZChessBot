#include "bitboard.hpp"
#include "search.hpp"

int main() {
	std::string fen;
	std::string tmp;
	int depth;
	std::getline(std::cin, fen);
	std::getline(std::cin, tmp);
	depth = std::stoi(tmp);
	Board board(fen);
	board.print_board();
	std::cout << ab_search(board, depth) << std::endl;
	return 0;
}
