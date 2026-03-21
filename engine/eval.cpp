#include "eval.hpp"

// Accumulator w_acc, b_acc;
Network nnue_network;

__attribute__((constructor)) void init_network() {
	nnue_network.load();
}

Value simple_eval(Position &pos) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * _mm_popcnt_u64(pos.piece_boards[i] & pos.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * _mm_popcnt_u64(pos.piece_boards[i] & pos.piece_boards[OCC(BLACK)]);
	}
	return score;
}

Value eval(Position &pos, AccumulatorManager &am) {
	int npieces = _mm_popcnt_u64(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	int32_t score = 0;

	am.apply_lazy(pos);

	int nbucket = (npieces - 2) / 4;

	if (pos.side == WHITE) {
		score = nnue_eval(nnue_network, am.current().w_acc, am.current().b_acc, nbucket);
	} else {
		score = -nnue_eval(nnue_network, am.current().b_acc, am.current().w_acc, nbucket);
	}

	return score;
}

std::array<Value, 8> debug_eval(Position &pos) {
	if (!(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return {VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (!(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return {-VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (pos.halfmove >= 100) {
		return {0, 0, 0, 0, 0, 0, 0, 0}; // Draw by 50 moves
	}

	Square wkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

	AccumulatorManager am(pos);

	int npieces = _mm_popcnt_u64(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);

	std::array<Value, 8> score = {};
	if (pos.side == WHITE) {
		for (int i = 0; i < 8; i++) {
			score[i] = nnue_eval(nnue_network, am.current().w_acc, am.current().b_acc, i);
		}
	} else {
		for (int i = 0; i < 8; i++) {
			score[i] = -nnue_eval(nnue_network, am.current().b_acc, am.current().w_acc, i);
		}
	}

	return score;
}
