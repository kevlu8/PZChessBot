#include "../fen.hpp"
#include "search.hpp"
#include <iostream>

int main() {
	char meta[5];
	std::string fen;
	std::getline(std::cin, fen);

	Board board = parse_fen(fen, meta);

	int depth;
	std::cin >> depth;
	std::string best = ab_search(&board, depth);
	std::cout << best << std::endl;
	return 0;
}
