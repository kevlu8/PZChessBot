#include "bitboard.hpp"

int main() {
	std::string fen;
	std::string tmp;
	int depth;
	std::getline(std::cin, fen);
	Board board(fen);
	board.print_board();
	std::unordered_set<uint16_t> moves;
	board.legal_moves(moves);
	std::cout << moves.size() << std::endl;
	for (uint16_t move : moves) {
		std::cout << (char)((move & 7) + 'a') << (char)(((move >> 3) & 7) + '1') << (char)(((move >> 6) & 7) + 'a') << (char)(((move >> 9) & 7) + '1');
		if (move & 0b1100000000000000)
			std::cout << piece_to_fen[((move >> 12) & 3) + 1];
		std::cout << std::endl;
	}

	// while (true) {
	// 	std::string move;
	// 	std::getline(std::cin, move);
	// 	uint16_t smove = 0;
	// 	smove = xyx(move[0] - 'a', move[1] - '1') | (xyx(move[2] - 'a', move[3] - '1') << 6);
	// 	if (move.length() == 5) {
	// 		smove |= (fen_to_piece[move[4] - 'b'] - 1) << 12;
	// 		smove |= 0b1100000000000000;
	// 	}
	// 	std::cout << smove << std::endl;
	// 	board.make_move(smove);
	// 	board.print_board();
	// }
	return 0;
}
