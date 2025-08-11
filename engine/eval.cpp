#include "eval.hpp"

// Accumulator w_acc, b_acc;
Network nnue_network;

extern Bitboard king_movetable[64];

static constexpr Value king_safety_lookup[9] = {-10, 20, 40, 50, 50, 50, 50, 50, 50};
static constexpr Value multipawn_lookup[7] = {0, 0, 20, 40, 80, 160, 320};

/**
 * @brief Boards to denote "good" squares for each piece type
 * @details The 8 boards map out an 8-bit signed binary number that represents how good or bad a square is for a piece type.
 * @details 127 is the best square for a piece, -128 is the worst.
 */
Bitboard PAWN_SQUARES[8];
Bitboard KNIGHT_SQUARES[8];
Bitboard BISHOP_SQUARES[8];
Bitboard ROOK_SQUARES[8];
Bitboard QUEEN_SQUARES[8];
Bitboard KING_SQUARES[8];
Bitboard KING_ENDGAME_SQUARES[8];
Bitboard PAWN_ENDGAME_SQUARES[8];

Bitboard PASSED_PAWN_MASKS[2][64];

__attribute__((constructor)) constexpr void gen_lookups() {
	// Convert heatmaps
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 64; j++) {
			PAWN_SQUARES[7 - i] |= (((Bitboard)pawn_heatmap[j] >> i) & 1) << j;
			KNIGHT_SQUARES[7 - i] |= (((Bitboard)knight_heatmap[j] >> i) & 1) << j;
			BISHOP_SQUARES[7 - i] |= (((Bitboard)bishop_heatmap[j] >> i) & 1) << j;
			ROOK_SQUARES[7 - i] |= (((Bitboard)rook_heatmap[j] >> i) & 1) << j;
			QUEEN_SQUARES[7 - i] |= (((Bitboard)queen_heatmap[j] >> i) & 1) << j;
			KING_SQUARES[7 - i] |= (((Bitboard)king_heatmap[j] >> i) & 1) << j;
			KING_ENDGAME_SQUARES[7 - i] |= (((Bitboard)endgame_heatmap[j] >> i) & 1) << j;
			PAWN_ENDGAME_SQUARES[7 - i] |= (((Bitboard)pawn_endgame[j] >> i) & 1) << j;
		}
	}

	for (int i = 8; i < 56; i++) {
		Bitboard white_mask = 0x0101010101010101ULL << (i + 8);
		Bitboard black_mask = 0x8080808080808080ULL >> (71 - i);
		PASSED_PAWN_MASKS[WHITE][i] = white_mask | ((white_mask << 1) & 0x0101010101010101) | ((white_mask >> 1) & 0x8080808080808080);
		PASSED_PAWN_MASKS[BLACK][i] = black_mask | ((black_mask << 1) & 0x0101010101010101) | ((black_mask >> 1) & 0x8080808080808080);
	}
}

__attribute__((constructor)) void init_network() {
#ifndef HCE
	nnue_network.load();
	for (int i = 0; i < HL_SIZE; i++) {
		bs.w_acc.val[i] = nnue_network.accumulator_biases[i];
		bs.b_acc.val[i] = nnue_network.accumulator_biases[i];
	}
#endif
}

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
	Value castling = 0;
	Value bishop_pair = 0;
	Value king_safety = 0;
	Value tempo_bonus = 0;
	Value pawn_structure = 0;

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
			case PAWN: piecesquare += side * pawn_heatmap[idx]; break;
			case KNIGHT: piecesquare += side * knight_heatmap[idx]; break;
			case BISHOP: piecesquare += side * bishop_heatmap[idx]; break;
			case ROOK: piecesquare += side * rook_heatmap[idx]; break;
			case QUEEN: piecesquare += side * queen_heatmap[idx]; break;
			case KING: piecesquare += side * king_heatmap[idx]; break;
		}
	}

	return material + piecesquare;
}

std::array<Value, 8> debug_eval(Board &board) {
	Value ev = eval(board);
	return {ev, ev, ev, ev, ev, ev, ev, ev};
}
