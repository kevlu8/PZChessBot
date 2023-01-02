#include "bitboard.hpp"

int main() {
	std::string fen;
	std::getline(std::cin, fen);
	Board board(fen);
	Board board2{board};
	board.print_board();
	std::unordered_set<uint16_t> moves;
	for (auto move : moves) {
		board2.make_move(move);
		std::unordered_set<uint16_t> moves2;
		for (auto move2 : moves2) {
			board2.make_move(move2);
			board2.unmake_move();
		}
		board2.unmake_move();
	}
	std::cout << '\n';
	board2.print_board();
	return 0;
}
