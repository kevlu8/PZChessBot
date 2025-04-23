#include "movegen.hpp"

struct MagicEntry {
	Bitboard mask;
	uint32_t offset;
};

Bitboard knight_movetable[64];
Bitboard king_movetable[64];
Bitboard rook_movetable[102400];
Bitboard bishop_movetable[5248];

MagicEntry rook_magics[64];
MagicEntry bishop_magics[64];

void gen_rook_moves(int sq, Bitboard piece) {
	Bitboard board = 0;
	int idx = 0;
	Bitboard rank = 0x00000000000000ff;
	Bitboard file = 0x0101010101010101;
	uint32_t offset;
	if (sq == 0)
		offset = 0;
	else
		offset = rook_magics[sq].offset;
	do {
		// Generate moves for this board (bitwise magic don't ask)
		Bitboard rankray = rank << (sq & 0b111000);
		Bitboard fileray = file << (sq & 0b111);

		Bitboard moves = 0;
		Bitboard west = rankray & (piece - 1);
		if (west & board)
			moves |= west & ~((1LL << (63 - _lzcnt_u64(west & board))) - 1);
		else
			moves |= west;
		Bitboard south = fileray & (piece - 1);
		if (south & board)
			moves |= south & ~((1LL << (63 - _lzcnt_u64(south & board))) - 1);
		else
			moves |= south;
		Bitboard east = rankray ^ west ^ piece;
		moves |= east & _blsmsk_u64(east & board);
		Bitboard north = fileray ^ south ^ piece;
		moves |= north & _blsmsk_u64(north & board);

		rook_movetable[offset + idx] = moves;
		// Prepare next board (this works i promise)
		board = (board - rook_magics[sq].mask) & rook_magics[sq].mask;
		idx++;
	} while (board);
	if (sq != 63)
		rook_magics[sq + 1].offset = offset + idx;
}

void gen_bishop_moves(int sq, Bitboard piece) {
	Bitboard board = 0;
	int idx = 0;
	Bitboard diag = 0x8040201008040201;
	Bitboard anti_diag = 0x0102040810204080;
	uint32_t offset;
	if (sq == 0)
		offset = 0;
	else
		offset = bishop_magics[sq].offset;
	do {
		// Generate moves for this board (bitwise magic don't ask)
		int shift = (sq & 0b111) - (sq >> 3);
		Bitboard diagray;
		if (shift >= 0)
			diagray = (diag >> (shift * 8));
		else
			diagray = (diag << (-shift * 8));
		Bitboard antiray;
		shift = 7 - (sq & 0b111) - (sq >> 3);
		if (shift >= 0)
			antiray = (anti_diag >> (shift * 8));
		else
			antiray = (anti_diag << (-shift * 8));

		Bitboard moves = 0;
		Bitboard sw = diagray & (piece - 1);
		if (sw & board)
			moves |= sw & ~((1LL << (63 - _lzcnt_u64(sw & board))) - 1);
		else
			moves |= sw;
		Bitboard se = antiray & (piece - 1);
		if (se & board)
			moves |= se & ~((1LL << (63 - _lzcnt_u64(se & board))) - 1);
		else
			moves |= se;
		Bitboard ne = diagray ^ sw ^ piece;
		moves |= ne & _blsmsk_u64(ne & board);
		Bitboard nw = antiray ^ se ^ piece;
		moves |= nw & _blsmsk_u64(nw & board);

		bishop_movetable[offset + idx] = moves;
		// Prepare next board (this works i promise)
		board = (board - bishop_magics[sq].mask) & bishop_magics[sq].mask;
		idx++;
	} while (board);
	if (sq != 63)
		bishop_magics[sq + 1].offset = offset + idx;
}

// This function is called before main()
__attribute__((constructor)) void init_movetables() {
	// Initialize trivial bitboards
	Bitboard rank = 0x00000000000000ff;
	Bitboard file = 0x0101010101010101;
	Bitboard diag = 0x8040201008040201;
	Bitboard anti_diag = 0x0102040810204080;
	Bitboard piece = square_bits(SQ_A1);
	for (int i = 0; piece; piece <<= 1, i++) {
		// Knight
		Bitboard hor1 = ((piece & ~FileHBits) << 1) | ((piece & ~FileABits) >> 1);
		Bitboard hor2 = ((piece & ~FileHBits & ~FileGBits) << 2) | ((piece & ~FileABits & ~FileBBits) >> 2);
		knight_movetable[i] = (hor1 << 16) | (hor1 >> 16) | (hor2 << 8) | (hor2 >> 8);

		// King
		Bitboard king = hor1 | piece;
		king_movetable[i] = (king | (king << 8) | (king >> 8)) ^ piece;

		// Create mask for rook
		Bitboard mask = (rank << (i & 0b111000)) ^ (file << (i & 0b111)); // XOR gets rid of the square itself
		// Remove squares on the edges (irrelevant for rook moves)
		if ((i & 0b111) != FILE_A)
			mask &= ~FileABits;
		if ((i >> 3) != RANK_1)
			mask &= ~Rank1Bits;
		if ((i & 0b111) != FILE_H)
			mask &= ~FileHBits;
		if ((i >> 3) != RANK_8)
			mask &= ~Rank8Bits;
		rook_magics[i].mask = mask;
		gen_rook_moves(i, piece);

		// Create mask for bishop
		int shift = (i & 0b111) - (i >> 3);
		if (shift >= 0)
			mask = (diag >> (shift * 8));
		else
			mask = (diag << (-shift * 8));
		shift = 7 - (i & 0b111) - (i >> 3);
		if (shift >= 0)
			mask ^= (anti_diag >> (shift * 8));
		else
			mask ^= (anti_diag << (-shift * 8));
		// Remove squares on the edges (irrelevant for bishop moves)
		if ((i & 0b111) != FILE_A)
			mask &= ~FileABits;
		if ((i >> 3) != RANK_1)
			mask &= ~Rank1Bits;
		if ((i & 0b111) != FILE_H)
			mask &= ~FileHBits;
		if ((i >> 3) != RANK_8)
			mask &= ~Rank8Bits;
		bishop_magics[i].mask = mask;
		gen_bishop_moves(i, piece);
	}
}

void white_pawn_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard pieces = board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)];
	Bitboard dsts;
	// En passant
	if (board.ep_square != SQ_NONE) {
		dsts = ((pieces & ~FileABits & Rank5Bits) << 7) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			moves.push_back(Move::make<EN_PASSANT>(sq - 7, sq));
			dsts = _blsr_u64(dsts);
		}
		dsts = ((pieces & ~FileHBits & Rank5Bits) << 9) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			moves.push_back(Move::make<EN_PASSANT>(sq - 9, sq));
			dsts = _blsr_u64(dsts);
		}
	}
	// Promotion
	dsts = ((pieces & Rank7Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, QUEEN));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, ROOK));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, KNIGHT));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, BISHOP));
		dsts = _blsr_u64(dsts);
	}
	// Captures
	dsts = ((pieces & ~FileABits) << 7) & board.piece_boards[OCC(BLACK)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		if (sq >= SQ_A8) {
			moves.push_back(Move::make<PROMOTION>(sq - 7, sq, QUEEN));
			moves.push_back(Move::make<PROMOTION>(sq - 7, sq, ROOK));
			moves.push_back(Move::make<PROMOTION>(sq - 7, sq, KNIGHT));
			moves.push_back(Move::make<PROMOTION>(sq - 7, sq, BISHOP));
		} else {
			moves.push_back(Move(sq - 7, sq));
		}
		dsts = _blsr_u64(dsts);
	}
	dsts = ((pieces & ~FileHBits) << 9) & board.piece_boards[OCC(BLACK)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		if (sq >= SQ_A8) {
			moves.push_back(Move::make<PROMOTION>(sq - 9, sq, QUEEN));
			moves.push_back(Move::make<PROMOTION>(sq - 9, sq, ROOK));
			moves.push_back(Move::make<PROMOTION>(sq - 9, sq, KNIGHT));
			moves.push_back(Move::make<PROMOTION>(sq - 9, sq, BISHOP));
		} else {
			moves.push_back(Move(sq - 9, sq));
		}
		dsts = _blsr_u64(dsts);
	}
	// Normal single pushes (no promotion)
	dsts = ((pieces & ~Rank7Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	Bitboard tmp = dsts;
	while (tmp) {
		int sq = _tzcnt_u64(tmp);
		moves.push_back(Move(sq - 8, sq));
		tmp = _blsr_u64(tmp);
	}
	// Double pushes
	dsts = ((dsts & Rank3Bits) << 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		moves.push_back(Move(sq - 16, sq));
		dsts = _blsr_u64(dsts);
	}
}

void black_pawn_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard pieces = board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)];
	Bitboard dsts;
	// En passant
	if (board.ep_square != SQ_NONE) {
		dsts = ((pieces & ~FileHBits & Rank4Bits) >> 7) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			moves.push_back(Move::make<EN_PASSANT>(sq + 7, sq));
			dsts = _blsr_u64(dsts);
		}
		dsts = ((pieces & ~FileABits & Rank4Bits) >> 9) & square_bits(board.ep_square);
		while (dsts) {
			int sq = _tzcnt_u64(dsts);
			moves.push_back(Move::make<EN_PASSANT>(sq + 9, sq));
			dsts = _blsr_u64(dsts);
		}
	}
	// Promotion
	dsts = ((pieces & Rank2Bits) >> 8) & ~(board.piece_boards[OCC(BLACK)] | board.piece_boards[OCC(WHITE)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, QUEEN));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, ROOK));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, KNIGHT));
		moves.push_back(Move::make<PROMOTION>(sq + 8, sq, BISHOP));
		dsts = _blsr_u64(dsts);
	}
	// Captures
	dsts = ((pieces & ~FileHBits) >> 7) & board.piece_boards[OCC(WHITE)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		if (sq <= SQ_H1) {
			moves.push_back(Move::make<PROMOTION>(sq + 7, sq, QUEEN));
			moves.push_back(Move::make<PROMOTION>(sq + 7, sq, ROOK));
			moves.push_back(Move::make<PROMOTION>(sq + 7, sq, KNIGHT));
			moves.push_back(Move::make<PROMOTION>(sq + 7, sq, BISHOP));
		} else {
			moves.push_back(Move(sq + 7, sq));
		}
		dsts = _blsr_u64(dsts);
	}
	dsts = ((pieces & ~FileABits) >> 9) & board.piece_boards[OCC(WHITE)];
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		if (sq <= SQ_H1) {
			moves.push_back(Move::make<PROMOTION>(sq + 9, sq, QUEEN));
			moves.push_back(Move::make<PROMOTION>(sq + 9, sq, ROOK));
			moves.push_back(Move::make<PROMOTION>(sq + 9, sq, KNIGHT));
			moves.push_back(Move::make<PROMOTION>(sq + 9, sq, BISHOP));
		} else {
			moves.push_back(Move(sq + 9, sq));
		}
		dsts = _blsr_u64(dsts);
	}
	// Normal single pushes (no promotion)
	dsts = ((pieces & ~Rank2Bits) >> 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	Bitboard tmp = dsts;
	while (tmp) {
		int sq = _tzcnt_u64(tmp);
		moves.push_back(Move(sq + 8, sq));
		tmp = _blsr_u64(tmp);
	}
	// Double pushes
	dsts = ((dsts & Rank6Bits) >> 8) & ~(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		moves.push_back(Move(sq + 16, sq));
		dsts = _blsr_u64(dsts);
	}
}

void pawn_moves(const Board &board, pzstd::vector<Move> &moves) {
	if (board.side == WHITE) {
		white_pawn_moves(board, moves);
	} else {
		black_pawn_moves(board, moves);
	}
}

void knight_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard pieces = board.piece_boards[KNIGHT] & board.piece_boards[OCC(board.side)];
	while (pieces) {
		int sq = _tzcnt_u64(pieces);
		Bitboard dsts = knight_movetable[sq] & ~board.piece_boards[OCC(board.side)];
		while (dsts) {
			int dst = _tzcnt_u64(dsts);
			moves.push_back(Move(sq, dst));
			dsts = _blsr_u64(dsts);
		}
		pieces = _blsr_u64(pieces);
	}
}

void bishop_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard pieces = (board.piece_boards[BISHOP] | board.piece_boards[QUEEN]) & board.piece_boards[OCC(board.side)];
	while (pieces) {
		int sq = _tzcnt_u64(pieces);
		uint32_t idx = bishop_magics[sq].offset + _pext_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)], bishop_magics[sq].mask);
		Bitboard dsts = bishop_movetable[idx] & ~board.piece_boards[OCC(board.side)];
		while (dsts) {
			int dst = _tzcnt_u64(dsts);
			moves.push_back(Move(sq, dst));
			dsts = _blsr_u64(dsts);
		}
		pieces = _blsr_u64(pieces);
	}
}

void rook_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard pieces = (board.piece_boards[ROOK] | board.piece_boards[QUEEN]) & board.piece_boards[OCC(board.side)];
	while (pieces) {
		int sq = _tzcnt_u64(pieces);
		uint32_t idx = rook_magics[sq].offset + _pext_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)], rook_magics[sq].mask);
		Bitboard dsts = rook_movetable[idx] & ~board.piece_boards[OCC(board.side)];
		while (dsts) {
			int dst = _tzcnt_u64(dsts);
			moves.push_back(Move(sq, dst));
			dsts = _blsr_u64(dsts);
		}
		pieces = _blsr_u64(pieces);
	}
}

void king_moves(const Board &board, pzstd::vector<Move> &moves) {
	Bitboard piece = board.piece_boards[KING] & board.piece_boards[OCC(board.side)];
	if (__builtin_expect(piece == 0, false))
		return;
	int sq = _tzcnt_u64(piece);
	// Castling
	if (board.side == WHITE && !board.control(SQ_E1).second) {
		if (board.castling & WHITE_OO) {
			if (!((board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) & (square_bits(SQ_F1) | square_bits(SQ_G1))) &&
				!board.control(SQ_F1).second)
				moves.push_back(Move::make<CASTLING>(SQ_E1, SQ_G1));
		}
		if (board.castling & WHITE_OOO) {
			if (!((board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) & (square_bits(SQ_D1) | square_bits(SQ_C1) | square_bits(SQ_B1))) &&
				!board.control(SQ_D1).second)
				moves.push_back(Move::make<CASTLING>(SQ_E1, SQ_C1));
		}
	} else if (board.side == BLACK && !board.control(SQ_E8).first) {
		if (board.castling & BLACK_OO) {
			if (!((board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) & (square_bits(SQ_F8) | square_bits(SQ_G8))) && !board.control(SQ_F8).first)
				moves.push_back(Move::make<CASTLING>(SQ_E8, SQ_G8));
		}
		if (board.castling & BLACK_OOO) {
			if (!((board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) & (square_bits(SQ_D8) | square_bits(SQ_C8) | square_bits(SQ_B8))) &&
				!board.control(SQ_D8).first)
				moves.push_back(Move::make<CASTLING>(SQ_E8, SQ_C8));
		}
	}
	// Normal moves
	Bitboard dsts = king_movetable[sq] & ~board.piece_boards[OCC(board.side)];
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		moves.push_back(Move(sq, dst));
		dsts = _blsr_u64(dsts);
	}
}

void Board::legal_moves(pzstd::vector<Move> &moves) const {
	rook_moves(*this, moves);
	bishop_moves(*this, moves);
	knight_moves(*this, moves);
	pawn_moves(*this, moves);
	king_moves(*this, moves);
}

std::pair<int, int> Board::control(int sq) const {
	int white = 0;
	int black = 0;

	uint32_t idx = rook_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], rook_magics[sq].mask);
	white += _mm_popcnt_u64(rook_movetable[idx] & (piece_boards[ROOK] | piece_boards[QUEEN]) & piece_boards[OCC(WHITE)]);
	black += _mm_popcnt_u64(rook_movetable[idx] & (piece_boards[ROOK] | piece_boards[QUEEN]) & piece_boards[OCC(BLACK)]);

	idx = bishop_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], bishop_magics[sq].mask);
	white += _mm_popcnt_u64(bishop_movetable[idx] & (piece_boards[BISHOP] | piece_boards[QUEEN]) & piece_boards[OCC(WHITE)]);
	black += _mm_popcnt_u64(bishop_movetable[idx] & (piece_boards[BISHOP] | piece_boards[QUEEN]) & piece_boards[OCC(BLACK)]);

	white += _mm_popcnt_u64(knight_movetable[sq] & piece_boards[KNIGHT] & piece_boards[OCC(WHITE)]);
	black += _mm_popcnt_u64(knight_movetable[sq] & piece_boards[KNIGHT] & piece_boards[OCC(BLACK)]);

	white += _mm_popcnt_u64(
		((square_bits(Square(sq - 9)) & 0x7f7f7f7f7f7f7f7f) | (square_bits(Square(sq - 7)) & 0xfefefefefefefefe)) & piece_boards[PAWN] &
		piece_boards[OCC(WHITE)]
	);
	black += _mm_popcnt_u64(
		((square_bits(Square(sq + 9)) & 0xfefefefefefefefe) | (square_bits(Square(sq + 7)) & 0x7f7f7f7f7f7f7f7f)) & piece_boards[PAWN] &
		piece_boards[OCC(BLACK)]
	);

	white += _mm_popcnt_u64(king_movetable[sq] & piece_boards[KING] & piece_boards[OCC(WHITE)]);
	black += _mm_popcnt_u64(king_movetable[sq] & piece_boards[KING] & piece_boards[OCC(BLACK)]);

	return {white, black};
}

Value Board::see(Square sq) {
	Value val = 0;

	Bitboard tmp;
	PieceType atk = NO_PIECETYPE; // Get the least valuable attacker
	Square atksq = SQ_NONE; // Get the square of the attacker
	uint32_t idx;

	if (side == WHITE)
		tmp = ((square_bits(Square(sq - 9)) & 0x7f7f7f7f7f7f7f7f) | (square_bits(Square(sq - 7)) & 0xfefefefefefefefe)) & piece_boards[PAWN] & piece_boards[OCC(WHITE)];
	else
		tmp = ((square_bits(Square(sq + 7)) & 0x7f7f7f7f7f7f7f7f) | (square_bits(Square(sq + 9)) & 0xfefefefefefefefe)) & piece_boards[PAWN] & piece_boards[OCC(WHITE)];
	if (tmp) {
		atk = PAWN;
		atksq = Square(__tzcnt_u64(tmp));
		goto found;
	}

	tmp = knight_movetable[sq] & piece_boards[KNIGHT] & piece_boards[OCC(side)];
	if (tmp) {
		atk = KNIGHT;
		atksq = Square(__tzcnt_u64(tmp));
		goto found;
	}

	idx = bishop_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], bishop_magics[sq].mask);
	tmp = bishop_movetable[idx] & piece_boards[BISHOP] & piece_boards[OCC(side)];
	if (tmp) {
		atk = BISHOP;
		atksq = Square(__tzcnt_u64(tmp));
		goto found;
	}

	idx = rook_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], rook_magics[sq].mask);
	tmp = rook_movetable[idx] & piece_boards[ROOK] & piece_boards[OCC(side)];
	if (tmp) {
		atk = ROOK;
		atksq = Square(__tzcnt_u64(tmp));
		goto found;
	}

	idx = bishop_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], bishop_magics[sq].mask);
	tmp = bishop_movetable[idx] & piece_boards[QUEEN] & piece_boards[OCC(side)];
	idx = rook_magics[sq].offset + _pext_u64(piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)], rook_magics[sq].mask);
	tmp |= rook_movetable[idx] & piece_boards[QUEEN] & piece_boards[OCC(side)];
	if (tmp) {
		atk = QUEEN;
		atksq = Square(__tzcnt_u64(tmp));
		goto found;
	}

found:
	if (atk != NO_PIECETYPE) {
		make_move(Move::make<MoveType::NORMAL>(atksq, sq));
		val = std::max(0, PieceValue[atk & 7] - see(sq));
		unmake_move();
	}
	return val;
}

Value Board::see_capture(Move move) {
	Value val = 0;
	PieceType victim = PieceType(mailbox[move.dst()] & 7);
	make_move(move);
	val = PieceValue[victim] - see(move.src());
	unmake_move();
	return val;
}

Bitboard rook_attacks(Square sq, Bitboard occ) {
	uint32_t idx = rook_magics[sq].offset + _pext_u64(occ, rook_magics[sq].mask);
	return rook_movetable[idx];
}

Bitboard bishop_attacks(Square sq, Bitboard occ) {
	uint32_t idx = bishop_magics[sq].offset + _pext_u64(occ, bishop_magics[sq].mask);
	return bishop_movetable[idx];
}

Bitboard queen_attacks(Square sq, Bitboard occ) {
	uint32_t idx = rook_magics[sq].offset + _pext_u64(occ, rook_magics[sq].mask);
	Bitboard rook = rook_movetable[idx];
	idx = bishop_magics[sq].offset + _pext_u64(occ, bishop_magics[sq].mask);
	Bitboard bishop = bishop_movetable[idx];
	return rook | bishop;
}

Bitboard knight_attacks(Square sq) {
	return knight_movetable[sq];
}

Bitboard king_attacks(Square sq) {
	return king_movetable[sq];
}

Bitboard pawn_attacks(Square sq, bool color) {
	if (color == WHITE)
		return ((square_bits(Square(sq + 7)) & 0x7f7f7f7f7f7f7f7f) | (square_bits(Square(sq + 9)) & 0xfefefefefefefefe));
	else
		return ((square_bits(Square(sq - 7)) & 0x7f7f7f7f7f7f7f7f) | (square_bits(Square(sq - 9)) & 0xfefefefefefefefe));
}
