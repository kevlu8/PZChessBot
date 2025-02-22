#include "bitboard.hpp"
#include "moves.hpp"

struct MagicEntry {
	Bitboard mask;
	uint32_t offset;
};

extern Bitboard bishop_movetable[5248];
extern MagicEntry bishop_magics[64];

int main() {
	// Initialize an open position with a rook somewhere
	Board board("8/8/8/8/8/8/8/R3K3 w Q - 0 1");
	board.print_board();
	std::cout << '\n';
	std::vector<Move> moves;
	board.legal_moves(moves);
	for (Move move : moves) {
		std::cout << move.to_string() << '\n';
	}
	board.make_move(moves.front());
	board.print_board();
}
