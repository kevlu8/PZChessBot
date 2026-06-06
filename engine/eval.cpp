#include "eval.hpp"

Network *nnue_networks;

__attribute__((constructor)) void init_network() {
	nnue_networks = new Network[1];
	nnue_networks[0].load();
}

Value simple_eval(Position &pos) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * arch::popcnt(pos.piece_boards[i] & pos.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * arch::popcnt(pos.piece_boards[i] & pos.piece_boards[OCC(BLACK)]);
	}
	return score;
}

Value eval(Position &pos, AccumulatorManager &am) {
	int npieces = arch::popcnt(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	int32_t score = 0;

	am.apply_lazy(pos);

	int nbucket = (npieces - 2) / 4;

	if (pos.side == WHITE) {
		score = nnue_eval(am.net, am.current().w_acc, am.current().b_acc, nbucket);
	} else {
		score = -nnue_eval(am.net, am.current().b_acc, am.current().w_acc, nbucket);
	}

	const int mat_phase = PawnValue * arch::popcnt(pos.piece_boards[PAWN]) + KnightValue * arch::popcnt(pos.piece_boards[KNIGHT]) +
						  BishopValue * arch::popcnt(pos.piece_boards[BISHOP]) + RookValue * arch::popcnt(pos.piece_boards[ROOK]) +
						  QueenValue * arch::popcnt(pos.piece_boards[QUEEN]);

	return score * (26500 + mat_phase) / 32768;
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

	Square wkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

	AccumulatorManager am(pos, nnue_networks[0]);

	int npieces = arch::popcnt(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);

	std::array<Value, 8> score = {};
	if (pos.side == WHITE) {
		for (int i = 0; i < 8; i++) {
			score[i] = nnue_eval(nnue_networks[0], am.current().w_acc, am.current().b_acc, i);
		}
	} else {
		for (int i = 0; i < 8; i++) {
			score[i] = -nnue_eval(nnue_networks[0], am.current().b_acc, am.current().w_acc, i);
		}
	}

	return score;
}
