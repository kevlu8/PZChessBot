#include "moves.hpp"

uint64_t knight_movetable[64];

void init_movetables() {
	// Initialize knight moves
	uint64_t knight = square_bits(SQ_A1);
	for (int i = 0; i < 64; i++) {
		uint64_t hor1 = ((knight & ~FileHBits) << 1) | ((knight & ~FileABits) >> 1);
		uint64_t hor2 = ((knight & ~FileHBits & ~FileGBits) << 1) | ((hor1 & ~FileABits & ~FileBBits) >> 1);
		knight_movetable[i] = (hor1 << 16) | (hor1 >> 16) | (hor2 << 8) | (hor2 >> 8);
	}
}

void white_pawn_moves(const Board &board, std::vector<Move> &moves) {
	uint64_t pieces = board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)];
	// Normal single pushes (no promotion)
	uint64_t dsts = ((pieces & ~Rank7Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	uint64_t tmp = dsts;
	while (tmp) {
		int sq = _tzcnt_u64(tmp);
		tmp ^= 1ULL << sq;
		moves.push_back(Move(sq - 8, sq));
	}
	// Double pushes
	dsts = ((dsts & Rank3Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq - 16, sq));
	}
	// Promotion
	dsts = ((pieces & Rank7Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, QUEEN));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, ROOK));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, KNIGHT));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, BISHOP));
	}
	// Captures
	dsts = ((pieces & ~FileABits) << 7) & board.piece_boards[OCC(BLACK)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq - 7, sq));
	}
	dsts = ((pieces & ~FileHBits) << 9) & board.piece_boards[OCC(BLACK)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq - 9, sq));
	}
	// En passant
	if (board.ep_square != -1) {
		dsts = ((pieces & ~FileABits) << 7) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			dsts ^= 1ULL << sq;
			moves.push_back(Move::make<EN_PASSANT>(sq - 7, sq));
		}
		dsts = ((pieces & ~FileHBits) << 9) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			dsts ^= 1ULL << sq;
			moves.push_back(Move::make<EN_PASSANT>(sq - 9, sq));
		}
	}
}

void black_pawn_moves(const Board &board, std::vector<Move> &moves) {
	uint64_t pieces = board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)];
	// Normal single pushes (no promotion)
	uint64_t dsts = ((pieces & ~Rank2Bits) >> 8) & ~(board.piece_boards[OCC(BLACK)] | board.piece_boards[OCC(WHITE)]);
	uint64_t tmp = dsts;
	while (tmp) {
		int sq = _tzcnt_u64(tmp);
		tmp ^= 1ULL << sq;
		moves.push_back(Move(sq + 8, sq));
	}
	// Double pushes
	dsts = ((dsts & Rank6Bits) >> 8) & ~(board.piece_boards[OCC(BLACK)] | board.piece_boards[OCC(WHITE)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq + 16, sq));
	}
	// Promotion
	dsts = ((pieces & Rank2Bits) >> 8) & ~(board.piece_boards[OCC(BLACK)] | board.piece_boards[OCC(WHITE)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, QUEEN));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, ROOK));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, KNIGHT));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, BISHOP));
	}
	// Captures
	dsts = ((pieces & ~FileHBits) << 7) & board.piece_boards[OCC(WHITE)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq + 7, sq));
	}
	dsts = ((pieces & ~FileABits) >> 9) & board.piece_boards[OCC(WHITE)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq + 9, sq));
	}
	// En passant
	if (board.ep_square != -1) {
		dsts = ((pieces & ~FileHBits) >> 7) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			dsts ^= 1ULL << sq;
			moves.push_back(Move::make<EN_PASSANT>(sq + 7, sq));
		}
		dsts = ((pieces & ~FileABits) >> 9) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			dsts ^= 1ULL << sq;
			moves.push_back(Move::make<EN_PASSANT>(sq + 9, sq));
		}
	}
}

void pawn_moves(const Board &board, std::vector<Move> &moves) {
	if (board.side == WHITE) {
		white_pawn_moves(board, moves);
	} else {
		black_pawn_moves(board, moves);
	}
}

void knight_moves(const Board &board, std::vector<Move> &moves) {
	uint64_t pieces = board.piece_boards[KNIGHT] & board.piece_boards[OCC(board.side)];
	while (pieces) {
		int sq = _tzcnt_u64(pieces);
		pieces ^= 1ULL << sq;
		uint64_t dsts = knight_movetable[sq] & ~board.piece_boards[OCC(board.side)];
		while (dsts) {
			int dst = _tzcnt_u64(dsts);
			dsts ^= 1ULL << dst;
			moves.push_back(Move(sq, dst));
		}
	}
}
