#include "eval.hpp"

// Accumulator w_acc, b_acc;
Network nnue_network;

__attribute__((constructor)) void init_network() {
	nnue_network.load();
}

Value simple_eval(Board &board) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}
	return score;
}

Value eval(Board &board, BoardState *bs) {
	// return -simple_eval(board);
	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	int32_t score = 0;
	// Query the NNUE network
	// Square wkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]);
	// Square bkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]);
	// int winbucket = IBUCKET_LAYOUT[wkingsq];
	// int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];
	int winbucket = 0, binbucket = 0;

	// Convert bs to usable format
	BoardState &state = *(bs + winbucket * NINPUTS * 2 + binbucket);

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		Piece prevpiece = state.mailbox[i];
		if (piece == prevpiece) continue;
		bool side = piece >> 3; // 1 = black, 0 = white
		bool prevside = prevpiece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		PieceType prevpt = PieceType(prevpiece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0, winbucket);
			accumulator_add(nnue_network, state.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1, binbucket);
			accumulator_add(nnue_network, state.b_acc, b_index);
		}

		if (prevpiece != NO_PIECE) {
			// Subtract from accumulator
			uint16_t w_index = calculate_index((Square)i, prevpt, prevside, 0, winbucket);
			accumulator_sub(nnue_network, state.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, prevpt, prevside, 1, binbucket);
			accumulator_sub(nnue_network, state.b_acc, b_index);
		}
	}

	memcpy(state.mailbox, board.mailbox, sizeof(state.mailbox));

	int nbucket = 0;

	if (board.side == WHITE) {
		score = nnue_eval(nnue_network, state.w_acc, state.b_acc, nbucket);
	} else {
		score = -nnue_eval(nnue_network, state.b_acc, state.w_acc, nbucket);
	}

	return score;
}

std::array<Value, 8> debug_eval(Board &board) {
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return {VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return {-VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (board.halfmove >= 100) {
		return {0, 0, 0, 0, 0, 0, 0, 0}; // Draw by 50 moves
	}

	Square wkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

	Accumulator w_acc, b_acc;
	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] = nnue_network.accumulator_biases[i];
		b_acc.val[i] = nnue_network.accumulator_biases[i];
	}

	// Query the NNUE network
	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0, winbucket);
			accumulator_add(nnue_network, w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1, binbucket);
			accumulator_add(nnue_network, b_acc, b_index);
		}
	}

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);

	std::array<Value, 8> score = {};
	if (board.side == WHITE) {
		for (int i = 0; i < 8; i++) {
			score[i] = nnue_eval(nnue_network, w_acc, b_acc, i);
		}
	} else {
		for (int i = 0; i < 8; i++) {
			score[i] = -nnue_eval(nnue_network, b_acc, w_acc, i);
		}
	}

	return score;
}
