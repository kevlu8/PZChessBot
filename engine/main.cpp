#include "bitboard.hpp"
#include "moves.hpp"

int main() {
	Board board;
	board.print_board();
	board.make_move(Move(0b0000001100011100));
	board.print_board();
}
