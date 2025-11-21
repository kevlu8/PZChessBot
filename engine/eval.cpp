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

__attribute__((constructor)) void init_network() {
#ifndef HCE
	nnue_network.load();
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

Value eval(Board &board, void *bs_ptr) {
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

	BoardState curstate = *((BoardState *)bs_ptr + (winbucket * NINPUTS * 2 + binbucket));
	// what the fuck??

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = board.mailbox[i];
		Piece prevpiece = curstate.mailbox[i];
		if (piece == prevpiece) continue;
		bool side = piece >> 3; // 1 = black, 0 = white
		bool prevside = prevpiece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		PieceType prevpt = PieceType(prevpiece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			uint16_t w_index = calculate_index((Square)i, pt, side, 0, winbucket);
			accumulator_add(nnue_network, curstate.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, pt, side, 1, binbucket);
			accumulator_add(nnue_network, curstate.b_acc, b_index);
		}

		if (prevpiece != NO_PIECE) {
			// Subtract from accumulator
			uint16_t w_index = calculate_index((Square)i, prevpt, prevside, 0, winbucket);
			accumulator_sub(nnue_network, curstate.w_acc, w_index);
			uint16_t b_index = calculate_index((Square)i, prevpt, prevside, 1, binbucket);
			accumulator_sub(nnue_network, curstate.b_acc, b_index);
		}
	}

	memcpy(curstate.mailbox, board.mailbox, sizeof(curstate.mailbox));

	int nbucket = (npieces - 2) / 4;

	if (board.side == WHITE) {
		score = nnue_eval(nnue_network, curstate.w_acc, curstate.b_acc, nbucket);
	} else {
		score = -nnue_eval(nnue_network, curstate.b_acc, curstate.w_acc, nbucket);
	}
	return score;
}
