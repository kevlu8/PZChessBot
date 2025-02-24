#include "eval.hpp"

/**
 * @brief Boards to denote "good" squares for each piece type
 * @details The 7 boards map out an 7-bit signed binary number that represents how good or bad a square is for a piece type.
 * @details 63 is the best square for a piece, -64 is the worst.
 */
Bitboard PAWN_SQUARES[8];
Bitboard KNIGHT_SQUARES[8];
Bitboard BISHOP_SQUARES[8];
Bitboard ROOK_SQUARES[8];
Bitboard QUEEN_SQUARES[8];
Bitboard KING_SQUARES[8];
Bitboard KING_ENDGAME_SQUARES[8];

__attribute__((constructor)) constexpr void init_heatmaps() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 64; j++) {
			PAWN_SQUARES[8-i] |= ((pawn_heatmap[j] >> i) & 1) << j;
			KNIGHT_SQUARES[8-i] |= ((knight_heatmap[j] >> i) & 1) << j;
			BISHOP_SQUARES[8-i] |= ((bishop_heatmap[j] >> i) & 1) << j;
			ROOK_SQUARES[8-i] |= ((rook_heatmap[j] >> i) & 1) << j;
			QUEEN_SQUARES[8-i] |= ((queen_heatmap[j] >> i) & 1) << j;
			KING_SQUARES[8-i] |= ((king_heatmap[j] >> i) & 1) << j;
			KING_ENDGAME_SQUARES[8-i] |= ((endgame_heatmap[j] >> i) & 1) << j;
		}
	}
}

Value eval(const Board &board) {
	/*
	1. Si une côté manque un roi, return MATE
	2. Compte les matérials de chaque côté (pondéré à 4)
	3. Prendre une total des cases controllées par chaque côté (pondéré à 2)
	*/

	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return VALUE_MATE;
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return -VALUE_MATE;
	}

	Value material = 0;
	Value controlled = 0;
	Value piecesquare = 0;

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

	// controlled += _mm_popcnt_u64(board.control[WHITE] & board.piece_boards[OCC(WHITE)]);
	// controlled -= _mm_popcnt_u64(board.control[BLACK] & board.piece_boards[OCC(BLACK)]);
	// controlled *= 10; // Full board control would be 64 * 10 = +640

	const Bitboard *funny = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) >= 8 ? KING_SQUARES : KING_ENDGAME_SQUARES;
	int8_t pawn_acc, knight_acc, bishop_acc, rook_acc, queen_acc, king_acc;

	for (int i = 0; i < 8; i++) {
		pawn_acc = pawn_acc * 2 + _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)] & PAWN_SQUARES[i]);
		knight_acc = knight_acc * 2 + _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(WHITE)] & KNIGHT_SQUARES[i]);
		bishop_acc = bishop_acc * 2 + _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)] & BISHOP_SQUARES[i]);
		rook_acc = rook_acc * 2 + _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(WHITE)] & ROOK_SQUARES[i]);
		queen_acc = queen_acc * 2 + _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(WHITE)] & QUEEN_SQUARES[i]);
		king_acc = king_acc * 2 + _mm_popcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)] & funny[i]);
	}
	piecesquare += pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc + king_acc;
	pawn_acc = knight_acc = bishop_acc = rook_acc = queen_acc = king_acc = 0;
	for (int i = 0; i < 8; i++) {
		pawn_acc = pawn_acc * 2 + _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)] & PAWN_SQUARES[i]);
		knight_acc = knight_acc * 2 + _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(BLACK)] & KNIGHT_SQUARES[i]);
		bishop_acc = bishop_acc * 2 + _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)] & BISHOP_SQUARES[i]);
		rook_acc = rook_acc * 2 + _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(BLACK)] & ROOK_SQUARES[i]);
		queen_acc = queen_acc * 2 + _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(BLACK)] & QUEEN_SQUARES[i]);
		king_acc = king_acc * 2 + _mm_popcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)] & funny[i]);
	}
	piecesquare -= pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc + king_acc;

	return ((int)material * 3 + (int)piecesquare) / 4;
}