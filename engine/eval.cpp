#include "eval.hpp"

Value simple_eval(Board &board) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}
	return score;
}

Value piece_value_mg[6] = {82, 337, 365, 477, 1025, 0};
Value piece_value_eg[6] = {94, 281, 297, 512, 936, 0};
Value taperer[6] = {0, 1, 1, 2, 4, 0};

Value mg_eval(Board &board, int &phase) {
	Value material = 0;
	Value piecesquare = 0;
	Value bishop_pair = 0;

	for (int i = 0; i < 6; i++) {
		material += piece_value_mg[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(WHITE)]);
		material -= piece_value_mg[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}

	for (int i = 0; i < 64; i++) {
		Piece p = board.mailbox[i];
		if (p == NO_PIECE) continue;
		int side = p >= 8 ? -1 : 1;
		PieceType pt = (PieceType)(p & 7);
		int idx = side == 1 ? i : (56 ^ i);
		phase += taperer[pt];
		switch (pt) {
			case PAWN: piecesquare += side * pawn_heatmap[idx]; break;
			case KNIGHT: piecesquare += side * knight_heatmap[idx]; break;
			case BISHOP: piecesquare += side * bishop_heatmap[idx]; break;
			case ROOK: piecesquare += side * rook_heatmap[idx]; break;
			case QUEEN: piecesquare += side * queen_heatmap[idx]; break;
			case KING: piecesquare += side * king_heatmap[idx]; break;
		}
	}

	bishop_pair += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]) >= 2 ? 10 : 0;
	bishop_pair -= _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]) >= 2 ? 10 : 0;

	return material + piecesquare + bishop_pair;
}

Value eg_eval(Board &board) {
	Value material = 0;
	Value piecesquare = 0;
	Value bishop_pair = 0;

	for (int i = 0; i < 6; i++) {
		material += piece_value_eg[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(WHITE)]);
		material -= piece_value_eg[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}

	for (int i = 0; i < 64; i++) {
		Piece p = board.mailbox[i];
		if (p == NO_PIECE) continue;
		int side = p >= 8 ? -1 : 1;
		PieceType pt = (PieceType)(p & 7);
		int idx = side == 1 ? i : (56 ^ i);
		switch (pt) {
			case PAWN: piecesquare += side * pawn_endgame[idx]; break;
			case KNIGHT: piecesquare += side * knight_endgame[idx]; break;
			case BISHOP: piecesquare += side * bishop_endgame[idx]; break;
			case ROOK: piecesquare += side * rook_endgame[idx]; break;
			case QUEEN: piecesquare += side * queen_endgame[idx]; break;
			case KING: piecesquare += side * king_endgame[idx]; break;
		}
	}

	bishop_pair += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]) >= 2 ? 10 : 0;
	bishop_pair -= _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]) >= 2 ? 10 : 0;

	return material + piecesquare + bishop_pair;
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

	int phase = 0;
	
	Value mg = mg_eval(board, phase);
	Value eg = eg_eval(board);

	if (phase > 24) phase = 24;

	Value fin_eval = (mg * phase + eg * (24 - phase)) / 24;
	return fin_eval;
}

std::array<Value, 8> debug_eval(Board &board) {
	Value ev = eval(board);
	return {ev, ev, ev, ev, ev, ev, ev, ev};
}
