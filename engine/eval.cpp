#include "eval.hpp"

Accumulator w_acc, b_acc;
Network nnue_network;

void init_network() {
	nnue_network.load("nnue.bin");
}

Value eval(Board &board) {
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return VALUE_MATE;
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return -VALUE_MATE;
	}
	
	// Query the NNUE network
	// Efficient updates are not implemented yet for debugging purposes
	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] = nnue_network.accumulator_biases[i];
		b_acc.val[i] = nnue_network.accumulator_biases[HL_SIZE + i];
	}

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		if (piece == NO_PIECE) continue;
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		uint16_t index = calculate_index((Square)i, pt, side, 0);
		if (side == WHITE) {
			accumulator_add(nnue_network, w_acc, index);
		} else {
			accumulator_add(nnue_network, b_acc, index);
		}
	}

	int32_t score = nnue_eval(nnue_network, w_acc, b_acc);
	return score;
}
