#include "eval.hpp"

extern Bitboard king_movetable[64];

static constexpr Value king_safety_lookup[9] = {-10, 20, 40, 50, 50, 50, 50, 50, 50};
static constexpr Value multipawn_lookup[7] = {0, 0, 20, 40, 80, 160, 320};

Bitboard PASSED_PAWN_MASKS[2][64];

__attribute__((constructor)) constexpr void gen_lookups() {
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
	// Initialize accumulators
	Value pawn_acc, knight_acc, bishop_acc, rook_acc, queen_acc, king_acc;
	Value pawn_acc_black, knight_acc_black, bishop_acc_black, rook_acc_black, queen_acc_black, king_acc_black;
	pawn_acc = knight_acc = bishop_acc = rook_acc = queen_acc = king_acc = 0;
	pawn_acc_black = knight_acc_black = bishop_acc_black = rook_acc_black = queen_acc_black = king_acc_black = 0;

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);

	for (int i = 0; i < 64; i++) {
		Piece p = board.mailbox[i];
		switch (p) {
		case WHITE_PAWN:
			if (npieces >= 10)
				pawn_acc += pawn_heatmap[i];
			else
				pawn_acc += pawn_endgame[i];
			break;
		case WHITE_KNIGHT:
			knight_acc += knight_heatmap[i];
			break;
		case WHITE_BISHOP:
			bishop_acc += bishop_heatmap[i];
			break;
		case WHITE_QUEEN:
			queen_acc += queen_heatmap[i];
			break;
		case WHITE_KING:
			if (npieces >= 10)
				king_acc += king_heatmap[i];
			else
				king_acc += endgame_heatmap[i];
			break;
		case BLACK_PAWN:
			if (npieces >= 10)
				pawn_acc -= pawn_heatmap[i ^ 56];
			else
				pawn_acc -= pawn_endgame[i ^ 56];
			break;
		case BLACK_KNIGHT:
			knight_acc -= knight_heatmap[i ^ 56];
			break;
		case BLACK_BISHOP:
			bishop_acc -= bishop_heatmap[i ^ 56];
			break;
		case BLACK_QUEEN:
			queen_acc -= queen_heatmap[i ^ 56];
			break;
		case BLACK_KING:
			if (npieces >= 10)
				king_acc -= king_heatmap[i ^ 56];
			else
				king_acc -= endgame_heatmap[i ^ 56];
			break;
		}
	}
	piecesquare += pawn_acc + knight_acc + bishop_acc + rook_acc + queen_acc + king_acc;
	
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

	return ((int)material * 3 + (int)piecesquare + (int)bishop_pair + (int)king_safety * 2 + (int)tempo_bonus + (int)pawn_structure) *
		   multi(npieces);
}

std::array<Value, 8> debug_eval(Board &board) {
	return {eval(board), 0, 0, 0, 0, 0, 0, 0};
}