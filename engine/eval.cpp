#include "eval.hpp"

extern Bitboard king_movetable[64];

static constexpr Value safety_lookup[9] = {-10, 20, 40, 50, 50, 50, 50, 50, 50};

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

__attribute__((constructor)) constexpr void init_heatmaps() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 64; j++) {
			PAWN_SQUARES[7-i] |= (((Bitboard)pawn_heatmap[j] >> i) & 1) << j;
			KNIGHT_SQUARES[7-i] |= (((Bitboard)knight_heatmap[j] >> i) & 1) << j;
			BISHOP_SQUARES[7-i] |= (((Bitboard)bishop_heatmap[j] >> i) & 1) << j;
			ROOK_SQUARES[7-i] |= (((Bitboard)rook_heatmap[j] >> i) & 1) << j;
			QUEEN_SQUARES[7-i] |= (((Bitboard)queen_heatmap[j] >> i) & 1) << j;
			KING_SQUARES[7-i] |= (((Bitboard)king_heatmap[j] >> i) & 1) << j;
			KING_ENDGAME_SQUARES[7-i] |= (((Bitboard)endgame_heatmap[j] >> i) & 1) << j;
			PAWN_ENDGAME_SQUARES[7-i] |= (((Bitboard)pawn_endgame[j] >> i) & 1) << j;
		}
	}
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

	// Decide between normal vs endgame king map
	const Bitboard *funny = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) >= 10 ? KING_SQUARES : KING_ENDGAME_SQUARES;
	const Bitboard *pawn = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) >= 10 ? PAWN_SQUARES : PAWN_ENDGAME_SQUARES;
	// Initialize accumulators
	int8_t pawn_acc, knight_acc, bishop_acc, rook_acc, queen_acc, king_acc;
	int8_t pawn_acc_black, knight_acc_black, bishop_acc_black, rook_acc_black, queen_acc_black, king_acc_black;
	pawn_acc = knight_acc = bishop_acc = rook_acc = queen_acc = king_acc = 0;
	pawn_acc_black = knight_acc_black = bishop_acc_black = rook_acc_black = queen_acc_black = king_acc_black = 0;

	// Precompute piece boards (flipping for black)
	Bitboard boards[12];
	for (int i = 0; i < 6; i++) {
		boards[i] = board.piece_boards[i] & board.piece_boards[OCC(WHITE)];
		boards[i+6] = __bswap_64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
	}

	for (int i = 0; i < 8; i++) {
		pawn_acc = pawn_acc * 2 + _mm_popcnt_u64(boards[0] & pawn[i]);
		knight_acc = knight_acc * 2 + _mm_popcnt_u64(boards[1] & KNIGHT_SQUARES[i]);
		bishop_acc = bishop_acc * 2 + _mm_popcnt_u64(boards[2] & BISHOP_SQUARES[i]);
		rook_acc = rook_acc * 2 + _mm_popcnt_u64(boards[3] & ROOK_SQUARES[i]);
		queen_acc = queen_acc * 2 + _mm_popcnt_u64(boards[4] & QUEEN_SQUARES[i]);
		king_acc = king_acc * 2 + _mm_popcnt_u64(boards[5] & funny[i]);

		pawn_acc_black = pawn_acc_black * 2 + _mm_popcnt_u64(boards[6] & pawn[i]);
		knight_acc_black = knight_acc_black * 2 + _mm_popcnt_u64(boards[7] & KNIGHT_SQUARES[i]);
		bishop_acc_black = bishop_acc_black * 2 + _mm_popcnt_u64(boards[8] & BISHOP_SQUARES[i]);
		rook_acc_black = rook_acc_black * 2 + _mm_popcnt_u64(boards[9] & ROOK_SQUARES[i]);
		queen_acc_black = queen_acc_black * 2 + _mm_popcnt_u64(boards[10] & QUEEN_SQUARES[i]);
		king_acc_black = king_acc_black * 2 + _mm_popcnt_u64(boards[11] & funny[i]);
	}
	piecesquare += pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc + king_acc;
	piecesquare -= pawn_acc_black + knight_acc_black + bishop_acc_black + rook_acc_black + queen_acc_black + king_acc_black;

	castling += (board.castling & WHITE_OO) ? 5 : 0;
	castling += (board.castling & WHITE_OOO) ? 5 : 0;
	castling -= (board.castling & BLACK_OO) ? 5 : 0;
	castling -= (board.castling & BLACK_OOO) ? 5 : 0;

	bishop_pair += _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]) >= 2 ? 30 : 0;
	bishop_pair -= _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]) >= 2 ? 30 : 0;

	// For king safety, check for opponent control on squares around the king
	// As well as counting our own pieces in front of the king
	
	king_safety += safety_lookup[_mm_popcnt_u64(king_movetable[__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])] & board.piece_boards[6])];
	king_safety -= safety_lookup[_mm_popcnt_u64(king_movetable[__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])] & board.piece_boards[7])];
	Bitboard white_king = (board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]);
	Bitboard black_king = (board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]);
	if (white_king & (square_bits(SQ_G1) | square_bits(SQ_H1))) {
		std::pair<int, int> control = board.control(SQ_F2);
		king_safety -= std::max(0, control.second - control.first) * 10;
		control = board.control(SQ_G2);
		king_safety -= std::max(0, control.second - control.first) * 15;
		control = board.control(SQ_H2);
		king_safety -= std::max(0, control.second - control.first) * 15;
	} else if (white_king & (square_bits(SQ_B1) | square_bits(SQ_C1))) {
		std::pair<int, int> control = board.control(SQ_A2);
		king_safety -= std::max(0, control.second - control.first) * 12;
		control = board.control(SQ_B2);
		king_safety -= std::max(0, control.second - control.first) * 15;
		control = board.control(SQ_C2);
		king_safety -= std::max(0, control.second - control.first) * 15;
	}

	if (black_king & (square_bits(SQ_G8) | square_bits(SQ_H8))) {
		std::pair<int, int> control = board.control(SQ_F7);
		king_safety += std::max(0, control.first - control.second) * 10;
		control = board.control(SQ_G7);
		king_safety += std::max(0, control.first - control.second) * 15;
		control = board.control(SQ_H7);
		king_safety += std::max(0, control.first - control.second) * 15;
	} else if (black_king & (square_bits(SQ_B8) | square_bits(SQ_C8))) {
		std::pair<int, int> control = board.control(SQ_A7);
		king_safety += std::max(0, control.first - control.second) * 12;
		control = board.control(SQ_B7);
		king_safety += std::max(0, control.first - control.second) * 15;
		control = board.control(SQ_C7);
		king_safety += std::max(0, control.first - control.second) * 15;
	}

	tempo_bonus += board.side == WHITE ? 10 : -10;

	return ((int)material * 3 + (int)piecesquare + (int)castling + (int)bishop_pair + (int)king_safety * 2 + (int)tempo_bonus);
}