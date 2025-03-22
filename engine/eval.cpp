#include "eval.hpp"

Accumulator w_acc, b_acc;
Network nnue_network;
Piece prev_mailbox[64] = {};

void init_network() {
	nnue_network.load("nnue.bin");
	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] = nnue_network.accumulator_biases[i];
		b_acc.val[i] = nnue_network.accumulator_biases[i];
	}
	std::fill(prev_mailbox, prev_mailbox + 64, NO_PIECE);
}

// Returns evaluation from white's perspective
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
	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		Piece prev_piece = prev_mailbox[i];
		if (piece == prev_piece) continue; // No change
		bool side = piece >> 3; // 1 = black, 0 = white
		bool prev_side = prev_piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		PieceType prev_pt = PieceType(prev_piece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0);
			accumulator_add(nnue_network, w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1);
			accumulator_add(nnue_network, b_acc, b_index);
		}

		if (prev_piece != NO_PIECE) {
			// Subtract from accumulator
			uint16_t w_index = calculate_index((Square)i, prev_pt, prev_side, 0);
			accumulator_sub(nnue_network, w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, prev_pt, prev_side, 1);
			accumulator_sub(nnue_network, b_acc, b_index);
		}
	}

	memcpy(prev_mailbox, board.mailbox, sizeof(prev_mailbox));

	// Temporarily disabled efficient updating to debug
	// for (int i = 0; i < HL_SIZE; i++) {
	// 	// Reset accumulator
	// 	w_acc.val[i] = nnue_network.accumulator_biases[i];
	// 	b_acc.val[i] = nnue_network.accumulator_biases[i];
	// }

	// for (uint16_t i = 0; i < 64; i++) {
	// 	Piece piece = board.mailbox[i];
	// 	PieceType pt = PieceType(piece & 7);
	// 	bool side = piece >> 3; // 1 = black, 0 = white
	// 	if (piece != NO_PIECE) {
	// 		// Add to accumulator
	// 		uint16_t w_index = calculate_index((Square)i, pt, side, 0);
	// 		accumulator_add(nnue_network, w_acc, w_index);
	// 		uint16_t b_index = calculate_index((Square)i, pt, side, 1);
	// 		accumulator_add(nnue_network, b_acc, b_index);
	// 	}
	// }

	int32_t score;
	if (board.side == WHITE) {
		score = nnue_eval(nnue_network, w_acc, b_acc);
	} else {
		score = -nnue_eval(nnue_network, b_acc, w_acc);
	}
	return score;
}
