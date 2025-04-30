#include "eval.hpp"

extern Bitboard king_movetable[64];

static constexpr Value king_safety_lookup[9] = {-10, 20, 40, 50, 50, 50, 50, 50, 50};
static constexpr Value multipawn_lookup[7] = {0, 0, 20, 40, 80, 160, 320};

/**
 * @brief Boards to denote "good" squares for each piece type
 * @details The 8 boards map out an 8-bit signed binary number that represents how good or bad a square is for a piece type.
 * @details 127 is the best square for a piece, -128 is the worst.
 */
Bitboard PAWN_SQUARES[16];
Bitboard KNIGHT_SQUARES[16];
Bitboard BISHOP_SQUARES[16];
Bitboard ROOK_SQUARES[16];
Bitboard QUEEN_SQUARES[16];
Bitboard KING_SQUARES[16];
Bitboard KING_ENDGAME_SQUARES[16];
Bitboard PAWN_ENDGAME_SQUARES[16];
Bitboard KNIGHT_ENDGAME_SQUARES[16];
Bitboard BISHOP_ENDGAME_SQUARES[16];
Bitboard ROOK_ENDGAME_SQUARES[16];
Bitboard QUEEN_ENDGAME_SQUARES[16];

Bitboard PASSED_PAWN_MASKS[2][64];

__attribute__((constructor)) constexpr void gen_lookups() {
	// Convert heatmaps
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 64; j++) {
			PAWN_SQUARES[15 - i] |= (((Bitboard)pawn_heatmap[j] >> i) & 1) << j;
			KNIGHT_SQUARES[15 - i] |= (((Bitboard)knight_heatmap[j] >> i) & 1) << j;
			BISHOP_SQUARES[15 - i] |= (((Bitboard)bishop_heatmap[j] >> i) & 1) << j;
			ROOK_SQUARES[15 - i] |= (((Bitboard)rook_heatmap[j] >> i) & 1) << j;
			QUEEN_SQUARES[15 - i] |= (((Bitboard)queen_heatmap[j] >> i) & 1) << j;
			KING_SQUARES[15 - i] |= (((Bitboard)king_heatmap[j] >> i) & 1) << j;
			KING_ENDGAME_SQUARES[15 - i] |= (((Bitboard)king_endgame[j] >> i) & 1) << j;
			PAWN_ENDGAME_SQUARES[15 - i] |= (((Bitboard)pawn_endgame[j] >> i) & 1) << j;
			KNIGHT_ENDGAME_SQUARES[15 - i] |= (((Bitboard)knight_endgame[j] >> i) & 1) << j;
			BISHOP_ENDGAME_SQUARES[15 - i] |= (((Bitboard)bishop_endgame[j] >> i) & 1) << j;
			ROOK_ENDGAME_SQUARES[15 - i] |= (((Bitboard)rook_endgame[j] >> i) & 1) << j;
			QUEEN_ENDGAME_SQUARES[15 - i] |= (((Bitboard)queen_endgame[j] >> i) & 1) << j;
		}
	}

	for (int i = 8; i < 56; i++) {
		Bitboard white_mask = 0x0101010101010101ULL << (i + 8);
		Bitboard black_mask = 0x8080808080808080ULL >> (71 - i);
		PASSED_PAWN_MASKS[WHITE][i] = white_mask | ((white_mask << 1) & 0x0101010101010101) | ((white_mask >> 1) & 0x8080808080808080);
		PASSED_PAWN_MASKS[BLACK][i] = black_mask | ((black_mask << 1) & 0x0101010101010101) | ((black_mask >> 1) & 0x8080808080808080);
	}
}

Value mg_eval(Board &board) {Value material = 0;
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

	// Initialize accumulators
	Value pawn_acc, knight_acc, bishop_acc, rook_acc, queen_acc, king_acc;
	Value pawn_acc_black, knight_acc_black, bishop_acc_black, rook_acc_black, queen_acc_black, king_acc_black;
	pawn_acc = knight_acc = bishop_acc = rook_acc = queen_acc = king_acc = 0;
	pawn_acc_black = knight_acc_black = bishop_acc_black = rook_acc_black = queen_acc_black = king_acc_black = 0;

	// Precompute piece boards (flipping for black)
	Bitboard boards[12];
	for (int i = 0; i < 6; i++) {
		boards[i] = board.piece_boards[i] & board.piece_boards[OCC(WHITE)];
#ifndef WINDOWS
		boards[i + 6] = __bswap_64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#else
		boards[i + 6] = _byteswap_ulong(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#endif
	}

	for (int i = 0; i < 16; i++) {
		pawn_acc = pawn_acc * 2 + _mm_popcnt_u64(boards[0] & PAWN_SQUARES[i]);
		knight_acc = knight_acc * 2 + _mm_popcnt_u64(boards[1] & KNIGHT_SQUARES[i]);
		bishop_acc = bishop_acc * 2 + _mm_popcnt_u64(boards[2] & BISHOP_SQUARES[i]);
		rook_acc = rook_acc * 2 + _mm_popcnt_u64(boards[3] & ROOK_SQUARES[i]);
		queen_acc = queen_acc * 2 + _mm_popcnt_u64(boards[4] & QUEEN_SQUARES[i]);
		king_acc = king_acc * 2 + _mm_popcnt_u64(boards[5] & KING_SQUARES[i]);

		pawn_acc_black = pawn_acc_black * 2 + _mm_popcnt_u64(boards[6] & PAWN_SQUARES[i]);
		knight_acc_black = knight_acc_black * 2 + _mm_popcnt_u64(boards[7] & KNIGHT_SQUARES[i]);
		bishop_acc_black = bishop_acc_black * 2 + _mm_popcnt_u64(boards[8] & BISHOP_SQUARES[i]);
		rook_acc_black = rook_acc_black * 2 + _mm_popcnt_u64(boards[9] & ROOK_SQUARES[i]);
		queen_acc_black = queen_acc_black * 2 + _mm_popcnt_u64(boards[10] & QUEEN_SQUARES[i]);
		king_acc_black = king_acc_black * 2 + _mm_popcnt_u64(boards[11] & KING_SQUARES[i]);
	}
	piecesquare += pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc + king_acc;
	piecesquare -= pawn_acc_black + knight_acc_black + bishop_acc_black + rook_acc_black + queen_acc_black + king_acc_black;

	return ((int)material + (int)piecesquare);
}

Value eg_eval(Board &board) {
	Value material = 0;
	Value piecesquare = 0;
	Value pawn_structure = 0;

	material += PawnValueEg * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)]);
	material += KnightValueEg * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(WHITE)]);
	material += BishopValueEg * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]);
	material += RookValueEg * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(WHITE)]);
	material += QueenValueEg * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(WHITE)]);
	material -= PawnValueEg * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)]);
	material -= KnightValueEg * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(BLACK)]);
	material -= BishopValueEg * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]);
	material -= RookValueEg * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(BLACK)]);
	material -= QueenValueEg * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(BLACK)]);

	// Initialize accumulators
	Value pawn_acc, knight_acc, bishop_acc, rook_acc, queen_acc;
	Value pawn_acc_black, knight_acc_black, bishop_acc_black, rook_acc_black, queen_acc_black;
	pawn_acc = knight_acc = bishop_acc = rook_acc = queen_acc = 0;
	pawn_acc_black = knight_acc_black = bishop_acc_black = rook_acc_black = queen_acc_black = 0;
	// Precompute piece boards (flipping for black)
	Bitboard boards[12];
	for (int i = 0; i < 6; i++) {
		boards[i] = board.piece_boards[i] & board.piece_boards[OCC(WHITE)];
#ifndef WINDOWS
		boards[i + 6] = __bswap_64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#else
		boards[i + 6] = _byteswap_ulong(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#endif
	}

	for (int i = 0; i < 16; i++) {
		pawn_acc = pawn_acc * 2 + _mm_popcnt_u64(boards[0] & PAWN_ENDGAME_SQUARES[i]);
		knight_acc = knight_acc * 2 + _mm_popcnt_u64(boards[1] & KNIGHT_ENDGAME_SQUARES[i]);
		bishop_acc = bishop_acc * 2 + _mm_popcnt_u64(boards[2] & BISHOP_ENDGAME_SQUARES[i]);
		rook_acc = rook_acc * 2 + _mm_popcnt_u64(boards[3] & ROOK_ENDGAME_SQUARES[i]);
		queen_acc = queen_acc * 2 + _mm_popcnt_u64(boards[4] & QUEEN_ENDGAME_SQUARES[i]);

		pawn_acc_black = pawn_acc_black * 2 + _mm_popcnt_u64(boards[6] & PAWN_ENDGAME_SQUARES[i]);
		knight_acc_black = knight_acc_black * 2 + _mm_popcnt_u64(boards[7] & KNIGHT_ENDGAME_SQUARES[i]);
		bishop_acc_black = bishop_acc_black * 2 + _mm_popcnt_u64(boards[8] & BISHOP_ENDGAME_SQUARES[i]);
		rook_acc_black = rook_acc_black * 2 + _mm_popcnt_u64(boards[9] & ROOK_ENDGAME_SQUARES[i]);
		queen_acc_black = queen_acc_black * 2 + _mm_popcnt_u64(boards[10] & QUEEN_ENDGAME_SQUARES[i]);
	}
	piecesquare += pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc;
	piecesquare -= pawn_acc_black + knight_acc_black + bishop_acc_black + rook_acc_black + queen_acc_black;

	return ((int)material + (int)piecesquare);
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

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	int phase = std::min(npieces, 24);

	Value mg = mg_eval(board), eg = 0;
	if (phase < 24) eg = eg_eval(board);

	return (mg * phase + eg * (24 - phase)) / 24;
}

std::array<Value, 8> debug_eval(Board &board) {
	return {eval(board), 0, 0, 0, 0, 0, 0, 0};
}