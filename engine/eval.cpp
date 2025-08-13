#include "eval.hpp"

Value simple_eval(Board &board) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * _mm_popcnt_u64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}
	return score;
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

	Value material = 0;
	Value piecesquare = 0;
	Value bishop_pair = 0;
	Value king_safety = 0;
	Value tempo_bonus = 0;
	Value pawn_structure = 0;

	Value phase = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	Value eg_weight = phase <= 16 ? 20 - phase : 0;

	material += PawnValue * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)]);
	material += KnightValue * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(WHITE)]);
	material += BishopValue * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]);
	material += RookValue * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(WHITE)]);
	material += QueenValue * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(WHITE)]);
	material -= PawnValue * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)]);
	material -= KnightValue * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(BLACK)]);
	material -= BishopValue * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]);
	material -= RookValue * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(BLACK)]);
	material -= QueenValue * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(BLACK)]);

	for (int i = 0; i < 64; i++) {
		Piece p = board.mailbox[i];
		if (p == NO_PIECE) continue;
		int side = p >= 8 ? -1 : 1;
		PieceType pt = (PieceType)(p & 7);
		int idx = side == 1 ? i : (56 ^ i);
		switch (pt) {
			case PAWN: piecesquare += side * ((pawn_heatmap[idx] * (24 - eg_weight)) + (pawn_endgame[idx] * eg_weight)) / 24; break;
			case KNIGHT: piecesquare += side * ((knight_heatmap[idx] * (24 - eg_weight)) + (knight_endgame[idx] * eg_weight)) / 24; break;
			case BISHOP: piecesquare += side * ((bishop_heatmap[idx] * (24 - eg_weight)) + (bishop_endgame[idx] * eg_weight)) / 24; break;
			case ROOK: piecesquare += side * ((rook_heatmap[idx] * (24 - eg_weight)) + (rook_endgame[idx] * eg_weight)) / 24; break;
			case QUEEN: piecesquare += side * ((queen_heatmap[idx] * (24 - eg_weight)) + (queen_endgame[idx] * eg_weight)) / 24; break;
			case KING: piecesquare += side * ((king_heatmap[idx] * (24 - eg_weight)) + (king_endgame[idx] * eg_weight)) / 24; break;
		}
	}

	bishop_pair += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]) >= 2 ? 10 : 0;
	bishop_pair -= _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]) >= 2 ? 10 : 0;

	return material + piecesquare + bishop_pair;
}

std::array<Value, 8> debug_eval(Board &board) {
	Value ev = eval(board);
	return {ev, ev, ev, ev, ev, ev, ev, ev};
}
