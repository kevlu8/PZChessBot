#include "eval.hpp"

// Accumulator w_acc, b_acc;
Network nnue_network;

#ifdef HCE
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

float multi(int x) {
	// If there are fewer pieces on the board, we should raise the magnitude of the eval
	// This allows for the engine to prioritize trading pieces when ahead, especially in the endgame
	// The main caveat is that this may cause the engine to draw by insufficient material
	int diff = std::min(32 - x, 20); // Number of pieces taken off the board
	return 1.0 + 0.02 * diff;
}
#endif

void init_network() {
#ifndef HCE
	nnue_network.load();
	for (int i = 0; i < HL_SIZE; i++) {
		bs.w_acc.val[i] = nnue_network.accumulator_biases[i];
		bs.b_acc.val[i] = nnue_network.accumulator_biases[i];
	}
#endif
}

#ifdef HCE
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
#ifndef WINDOWS
		boards[i + 6] = __bswap_64(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#else
		boards[i + 6] = _byteswap_ulong(board.piece_boards[i] & board.piece_boards[OCC(BLACK)]);
#endif
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

	king_safety += king_safety_lookup[_mm_popcnt_u64(
		king_movetable[__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])] & board.piece_boards[OCC(WHITE)]
	)];
	king_safety -= king_safety_lookup[_mm_popcnt_u64(
		king_movetable[__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])] & board.piece_boards[OCC(BLACK)]
	)];
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

	for (Bitboard mask = 0x0101010101010101; mask & 0xff; mask <<= 1) {
		// Doubled pawns
		pawn_structure -= multipawn_lookup[_mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)] & mask)];
		pawn_structure += multipawn_lookup[_mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)] & mask)];

		// Isolated pawns
		if (mask & 0b01111110) {
			if ((board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)] & ((mask << 1) | (mask >> 1))) == 0)
				pawn_structure -= _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)] & mask) ? 60 : 0;
			if ((board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)] & ((mask << 1) | (mask >> 1))) == 0)
				pawn_structure += _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)] & mask) ? 60 : 0;
		}
	}

	Bitboard pawns = board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)];
	while (pawns) {
		int sq = _tzcnt_u64(pawns);
		pawn_structure += (board.piece_boards[PAWN] & PASSED_PAWN_MASKS[WHITE][sq]) == 0 ? 80 : 0;
		pawns = _blsr_u64(pawns);
	}
	pawns = board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)];
	while (pawns) {
		int sq = _tzcnt_u64(pawns);
		pawn_structure -= (board.piece_boards[PAWN] & PASSED_PAWN_MASKS[BLACK][sq]) == 0 ? 80 : 0;
		pawns = _blsr_u64(pawns);
	}

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);

	return ((int)material * 3 + (int)piecesquare + (int)castling + (int)bishop_pair + (int)king_safety * 2 + (int)tempo_bonus + (int)pawn_structure) *
		   multi(npieces);
}

std::array<Value, 8> debug_eval(Board &board) {
	return {eval(board), 0, 0, 0, 0, 0, 0, 0};
}
#else
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
	int32_t score = 0;
	// Query the NNUE network
	Square wkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

	if (winbucket != bs.wbucket || binbucket != bs.bbucket) {
		for (int i = 0; i < HL_SIZE; i++) {
			bs.w_acc.val[i] = nnue_network.accumulator_biases[i];
			bs.b_acc.val[i] = nnue_network.accumulator_biases[i];
		}
		for (int i = 0; i < 64; i++) bs.mailbox[i] = NO_PIECE;
		bs.wbucket = winbucket;
		bs.bbucket = binbucket;
	}

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		Piece prevpiece = bs.mailbox[i];
		if (piece == prevpiece) continue;
		bool side = piece >> 3; // 1 = black, 0 = white
		bool prevside = prevpiece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		PieceType prevpt = PieceType(prevpiece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0, winbucket);
			accumulator_add(nnue_network, bs.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1, binbucket);
			accumulator_add(nnue_network, bs.b_acc, b_index);
		}

		if (prevpiece != NO_PIECE) {
			// Subtract from accumulator
			uint16_t w_index = calculate_index((Square)i, prevpt, prevside, 0, winbucket);
			accumulator_sub(nnue_network, bs.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, prevpt, prevside, 1, binbucket);
			accumulator_sub(nnue_network, bs.b_acc, b_index);
		}
	}

	memcpy(bs.mailbox, board.mailbox, sizeof(bs.mailbox));

	int nbucket = (npieces - 2) / 4;

	if (board.side == WHITE) {
		score = nnue_eval(nnue_network, bs.w_acc, bs.b_acc, nbucket);
	} else {
		score = -nnue_eval(nnue_network, bs.b_acc, bs.w_acc, nbucket);
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

	if (winbucket != bs.wbucket || binbucket != bs.bbucket) {
		for (int i = 0; i < HL_SIZE; i++) {
			bs.w_acc.val[i] = nnue_network.accumulator_biases[i];
			bs.b_acc.val[i] = nnue_network.accumulator_biases[i];
		}
		for (int i = 0; i < 64; i++) bs.mailbox[i] = NO_PIECE;
		bs.wbucket = winbucket;
		bs.bbucket = binbucket;
	}

	// Query the NNUE network
	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		Piece prevpiece = bs.mailbox[i];
		if (piece == prevpiece)
			continue;
		bool side = piece >> 3; // 1 = black, 0 = white
		bool prevside = prevpiece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		PieceType prevpt = PieceType(prevpiece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0, winbucket);
			accumulator_add(nnue_network, bs.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1, binbucket);
			accumulator_add(nnue_network, bs.b_acc, b_index);
		}
		
		if (prevpiece != NO_PIECE) {
			// Subtract from accumulator
			uint16_t w_index = calculate_index((Square)i, prevpt, prevside, 0, winbucket);
			accumulator_sub(nnue_network, bs.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, prevpt, prevside, 1, binbucket);
			accumulator_sub(nnue_network, bs.b_acc, b_index);
		}
	}

	memcpy(bs.mailbox, board.mailbox, sizeof(bs.mailbox));

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);

	std::array<Value, 8> score = {};
	if (board.side == WHITE) {
		for (int i = 0; i < 8; i++) {
			score[i] = nnue_eval(nnue_network, bs.w_acc, bs.b_acc, i);
		}
	} else {
		for (int i = 0; i < 8; i++) {
			score[i] = -nnue_eval(nnue_network, bs.b_acc, bs.w_acc, i);
		}
	}

	return score;
}
#endif
