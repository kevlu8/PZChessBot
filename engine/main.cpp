#include "bitboard.hpp"
#include "moves.hpp"

int main() {
	Board board("8/8/8/8/8/8/8/R3K3 w - e3 0 1");
	board.print_board();
	std::cout << std::endl;
	board.make_move(Move::make<CASTLING>(SQ_E1, SQ_C1));
	board.print_board();
}
