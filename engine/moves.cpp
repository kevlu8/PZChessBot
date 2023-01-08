#include "bitboard.hpp"

U64 rankMask(int sq) { return C64(0xff) << (sq & 56); }

U64 fileMask(int sq) { return C64(0x0101010101010101) << (sq & 7); }

U64 diagonalMask(int sq) {
	const U64 maindia = C64(0x8040201008040201);
	int diag = 8 * (sq & 7) - (sq & 56);
	int nort = -diag & (diag >> 31);
	int sout = diag & (-diag >> 31);
	return (maindia >> sout) << nort;
}

U64 antiDiagMask(int sq) {
	const U64 maindia = C64(0x0102040810204080);
	int diag = 56 - 8 * (sq & 7) - (sq & 56);
	int nort = -diag & (diag >> 31);
	int sout = diag & (-diag >> 31);
	return (maindia >> sout) << nort;
}

void print_bits(U64 bits) {
	for (int j = 7; j >= 0; j--) {
		for (int i = 0; i < 8; i++) {
			std::cout << ((bits & BITxy(i, j)) ? 1 : 0);
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

U64 Board::king_control(const bool side) {
	// get the right king
	U64 king = pieces[0] & pieces[7 ^ side];

	// move west one
	king |= (king & C64(0xfefefefefefefefe)) >> 1;
	// move east one
	king |= (king & C64(0x7f7f7f7f7f7f7f7f)) << 1;
	// move north one
	king |= (king & C64(0x00ffffffffffffff)) << 8;
	// move south one
	king |= (king & C64(0xffffffffffffff00)) >> 8;
	// remove the king
	king ^= pieces[0] & pieces[7 ^ side];

	return king;
}

U64 Board::rook_control(const bool side) {
	// get the right set of rooks (and queens)
	U64 rooks = (pieces[2] | pieces[1]) & pieces[7 ^ side];
	// get a set of all occupied squares
	U64 occupied = pieces[6] | pieces[7];
	U64 control = 0;
	while (rooks) {
		uint8_t i = __builtin_ctzll(rooks);
		// get the rank and file masks
		U64 rankray = rankMask(i);
		U64 fileray = fileMask(i);

		// TODO: optimize the next 2 sections using SIMD instructions

		// the west ray is the part of the rank ray that comes before the rook
		U64 west = rankray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		control |= west & EXPANDMSB(west & occupied);
		// the southeast ray is the part of the file ray that comes after the rook
		U64 south = fileray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		control |= south & EXPANDMSB(south & occupied);

		// the east ray is the part of the rank ray that comes after the rook
		U64 east = rankray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		control |= east & EXPANDLSB(east & occupied);
		// the north ray is the part of the file ray that comes before the rook
		U64 north = fileray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		control |= north & EXPANDLSB(north & occupied);

		rooks &= rooks - 1;
	}
	return control;
}

U64 Board::bishop_control(const bool side) {
	// get the right set of bishops (and queens)
	U64 bishops = (pieces[3] | pieces[1]) & pieces[7 ^ side];
	// get a set of all occupied squares
	U64 occupied = pieces[6] | pieces[7];
	U64 control = 0;
	while (bishops) {
		uint8_t i = __builtin_ctzll(bishops);
		// get the rank and file masks
		U64 diagray = diagonalMask(i);
		U64 antidiagray = antiDiagMask(i);

		// TODO: optimize the next 2 sections using SIMD instructions

		// the west ray is the part of the rank ray that comes before the rook
		U64 southwest = diagray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		control |= southwest & EXPANDMSB(southwest & occupied);
		// the southeast ray is the part of the file ray that comes after the rook
		U64 southeast = antidiagray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		control |= southeast & EXPANDMSB(southeast & occupied);

		// the east ray is the part of the rank ray that comes after the rook
		U64 northeast = diagray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		control |= northeast & EXPANDLSB(northeast & occupied);
		// the north ray is the part of the file ray that comes before the rook
		U64 northwest = antidiagray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		control |= northwest & EXPANDLSB(northwest & occupied);

		bishops &= bishops - 1;
	}
	return control;
}

U64 Board::knight_control(const bool side) {
	// get the right set of knights
	U64 knights = pieces[4] & pieces[7 ^ side];

	// left one
	U64 l1 = (knights & C64(0xfefefefefefefefe)) >> 1;
	// left two
	U64 l2 = (knights & C64(0xfcfcfcfcfcfcfcfc)) >> 2;
	// right one
	U64 r1 = (knights & C64(0x7f7f7f7f7f7f7f7f)) << 1;
	// right two
	U64 r2 = (knights & C64(0x3f3f3f3f3f3f3f3f)) << 2;
	// one horizontal
	U64 h1 = l1 | r1;
	// two horizontal
	U64 h2 = l2 | r2;
	// one horizontal must mean two vertical
	// two horizontal must mean one vertical
	return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}

U64 Board::white_pawn_control() {
	U64 pawns = pieces[5] & pieces[6];
	return ((pawns & C64(0x00fefefefefefe00)) << 7) | ((pawns & C64(0x007f7f7f7f7f7f00)) << 9);
}

U64 Board::black_pawn_control() {
	U64 pawns = pieces[5] & pieces[7];
	return ((pawns & C64(0x00fefefefefefe00)) >> 9) | ((pawns & C64(0x007f7f7f7f7f7f00)) >> 7);
}

void Board::controlled_squares(const bool side) {
	U64 control = 0;
	control |= king_control(side);
	control |= rook_control(side);
	control |= bishop_control(side);
	control |= knight_control(side);
	if (side) {
		control |= white_pawn_control();
		this->control[side] = control;
	} else {
		control |= black_pawn_control();
		this->control[side] = control;
	}
}

void Board::king_moves(std::unordered_set<uint16_t> &out) {
	// get the right king
	U64 king = pieces[0] & pieces[7 ^ meta[0]];

	controlled_squares(!meta[0]);

	U64 moves = king;
	// move west one
	moves |= (moves & C64(0xfefefefefefefefe)) >> 1;
	// move east one
	moves |= (moves & C64(0x7f7f7f7f7f7f7f7f)) << 1;
	// move north one
	moves |= (moves & C64(0x00ffffffffffffff)) << 8;
	// move south one
	moves |= (moves & C64(0xffffffffffffff00)) >> 8;

	// remove the squares that are occupied by the same side
	moves &= ~pieces[7 ^ meta[0]];

	U64 captures = moves & pieces[6 ^ meta[0]];
	// remove the captures from the moves
	moves ^= captures;

	king = __builtin_ctzll(king);
	while (captures) {
		out.insert(king | (__builtin_ctzll(captures) << 6) | (0b0100 << 12));
		captures &= captures - 1;
	}
	while (moves) {
		out.insert(king | (__builtin_ctzll(moves) << 6));
		moves &= moves - 1;
	}

	// castling
	if ((meta[1] >> (meta[0] * 2)) & 0b01 && (((pieces[6] | pieces[7]) >> (king - 3)) & 0b111) == 0 && ((control[!meta[0]] >> (king - 2)) & 0b111) == 0) // queenside
		out.insert(0b0011111010111100 ^ (0b0000111000111000 * meta[0]));
	if ((meta[1] >> (meta[0] * 2)) & 0b10 && (((pieces[6] | pieces[7]) >> (king + 1)) & 0b11) == 0 && ((control[!meta[0]] >> king) & 0b111) == 0) // kingside
		out.insert(0b0010111110111100 ^ (0b0000111000111000 * meta[0]));
}

void Board::rook_moves(std::unordered_set<uint16_t> &out) {
	// get the right set of rooks (and queens)
	U64 rooks = (pieces[2] | pieces[1]) & pieces[7 ^ meta[0]];
	// get a set of all occupied squares
	U64 occupied = pieces[6] | pieces[7];
	while (rooks) {
		uint8_t i = __builtin_ctzll(rooks);
		// get the rank and file masks
		U64 rankray = rankMask(i);
		U64 fileray = fileMask(i);

		// TODO: optimize the next 2 sections using SIMD instructions

		// the west ray is the part of the rank ray that comes before the rook
		U64 west = rankray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		west &= EXPANDMSB(west & occupied);
		// the southeast ray is the part of the file ray that comes after the rook
		U64 south = fileray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		south &= EXPANDMSB(south & occupied);

		// the east ray is the part of the rank ray that comes after the rook
		U64 east = rankray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		east &= EXPANDLSB(east & occupied);
		// the north ray is the part of the file ray that comes before the rook
		U64 north = fileray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		north &= EXPANDLSB(north & occupied);

		// the valid moves are the union of all the rays except the squares that are occupied by the same color
		U64 moves = (west | south | east | north) & ~pieces[7 ^ meta[0]];
		U64 captures = moves & pieces[6 ^ meta[0]];
		moves ^= captures;

		rooks &= rooks - 1;

		while (captures) {
			out.insert(i | (__builtin_ctzll(captures) << 6) | (0b0100 << 12));
			captures &= captures - 1;
		}
		while (moves) {
			out.insert(i | (__builtin_ctzll(moves) << 6));
			moves &= moves - 1;
		}
	}
}

void Board::bishop_moves(std::unordered_set<uint16_t> &out) {
	// get the right set of bishops (and queens)
	U64 bishops = (pieces[3] | pieces[1]) & pieces[7 ^ meta[0]];
	// get a set of all occupied squares
	U64 occupied = pieces[6] | pieces[7];
	while (bishops) {
		uint8_t i = __builtin_ctzll(bishops);
		// get the diagonal and antidiagonal masks
		U64 diagray = diagonalMask(i);
		U64 antidiagray = antiDiagMask(i);

		// TODO: optimize the next 2 sections using SIMD instructions

		// the southwest ray is the part of the diagonal ray that comes before the bishop
		U64 swray = diagray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		swray &= EXPANDMSB(swray & occupied);
		// the southeast ray is the part of the antidiagonal ray that comes after the bishop
		U64 seray = antidiagray & LSONES(i);
		// remove everything before the last square in the intersection of the ray and the occupied squares
		seray &= EXPANDMSB(seray & occupied);

		// the northeast ray is the part of the diagonal ray that comes after the bishop
		U64 neray = diagray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		neray &= EXPANDLSB(neray & occupied);
		// the northwest ray is the part of the antidiagonal ray that comes before the bishop
		U64 nwray = antidiagray & MSONES(63 - i);
		// remove everything after the first square in the intersection of the ray and the occupied squares
		nwray &= EXPANDLSB(nwray & occupied);

		// the valid moves are the union of all the rays except the squares that are occupied by the same color
		U64 moves = (swray | seray | neray | nwray) & ~pieces[7 ^ meta[0]];
		U64 captures = moves & pieces[6 ^ meta[0]];
		moves ^= captures;

		bishops &= bishops - 1;

		while (captures) {
			out.insert(i | (__builtin_ctzll(captures) << 6) | (0b0100 << 12));
			captures &= captures - 1;
		}
		while (moves) {
			out.insert(i | (__builtin_ctzll(moves) << 6));
			moves &= moves - 1;
		}
	}
}

void Board::knight_moves(std::unordered_set<uint16_t> &out) {
	// get the right set of knights
	U64 knights = pieces[4] & pieces[7 ^ meta[0]];
	while (knights) {
		U64 knight = knights & -knights;
		U64 moves = 0;

		// TODO: optimize the following using parallel prefixes

		// south west west
		moves |= (knight & C64(0xfcfcfcfcfcfcfc00)) >> 10;
		// south south west
		moves |= (knight & C64(0xfefefefefefe0000)) >> 17;
		// south south east
		moves |= (knight & C64(0x7f7f7f7f7f7f0000)) >> 15;
		// south east east
		moves |= (knight & C64(0x3f3f3f3f3f3f3f00)) >> 6;
		// north east east
		moves |= (knight & C64(0x003f3f3f3f3f3f3f)) << 10;
		// north north east
		moves |= (knight & C64(0x00007f7f7f7f7f7f)) << 17;
		// north north west
		moves |= (knight & C64(0x0000fefefefefefe)) << 15;
		// north west west
		moves |= (knight & C64(0x00fcfcfcfcfcfcfc)) << 6;

		// remove the moves that are occupied by the same color
		moves &= ~pieces[7 ^ meta[0]];
		U64 captures = moves & pieces[6 ^ meta[0]];
		moves ^= captures;

		knights ^= knight;

		while (captures) {
			out.insert(__builtin_ctzll(knight) | (__builtin_ctzll(captures) << 6) | (0b0100 << 12));
			captures &= captures - 1;
		}
		while (moves) {
			out.insert(__builtin_ctzll(knight) | (__builtin_ctzll(moves) << 6));
			moves &= moves - 1;
		}
	}
}

void Board::white_pawn_moves(std::unordered_set<uint16_t> &out) {
	// get the right set of pawns
	U64 pawns = pieces[5] & pieces[6];

	// normal moves
	U64 moves = pawns & (~(pieces[6] | pieces[7]) >> 8);
	U64 promote = moves & C64(0x00ff000000000000);
	// remove duplicates caused by promotion
	moves &= C64(0x0000ffffffffff00);
	while (moves) {
		uint8_t i = __builtin_ctzll(moves);
		out.insert(i | ((i + 8) << 6));
		moves &= moves - 1;
	}
	while (promote) {
		uint8_t i = __builtin_ctzll(promote);
		out.insert(i | ((i + 8) << 6) | (0b1000 << 12));
		out.insert(i | ((i + 8) << 6) | (0b1001 << 12));
		out.insert(i | ((i + 8) << 6) | (0b1010 << 12));
		out.insert(i | ((i + 8) << 6) | (0b1011 << 12));
		promote &= promote - 1;
	}
	// double moves
	moves = pawns & (~(pieces[6] | pieces[7]) >> 8) & C64(0x000000000000ff00);
	moves &= ~(pieces[6] | pieces[7]) >> 16;
	while (moves) {
		uint8_t i = __builtin_ctzll(moves);
		out.insert(i | ((i + 16) << 6) | (0b0001 << 12));
		moves &= moves - 1;
	}
	// captures
	while (pawns) {
		U64 pawn = pawns & -pawns;
		moves = 0;
		// take west
		moves |= (pawn & C64(0x00fefefefefefe00)) << 7;
		// take east
		moves |= (pawn & C64(0x007f7f7f7f7f7f00)) << 9;
		// remove the moves that are not enemy pieces
		moves &= pieces[7];
		promote = moves & C64(0xff00000000000000);
		// remove duplicates caused by promotion
		moves &= C64(0x00ffffffffff0000);

		// en passant west
		moves |= ((pawn & C64(0x000000fe00000000)) << 7) & BIT(meta[2]);
		// en passant east
		moves |= ((pawn & C64(0x0000007f00000000)) << 9) & BIT(meta[2]);

		pawns &= pawns - 1;

		while (moves) {
			uint8_t j = __builtin_ctzll(moves);
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b0100 << 12) | ((j == meta[2]) << 12));
			moves &= moves - 1;
		}
		while (promote) {
			uint8_t j = __builtin_ctzll(promote);
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1100 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1101 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1110 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1111 << 12));
		}
	}
}

void Board::black_pawn_moves(std::unordered_set<uint16_t> &out) {
	// get the right set of pawns
	U64 pawns = pieces[5] & pieces[7];

	// normal moves
	U64 moves = pawns & (~(pieces[6] | pieces[7]) << 8);
	U64 promote = moves & C64(0x000000000000ff00);
	// remove duplicates caused by promotion
	moves &= C64(0x00ffffffffff0000);
	while (moves) {
		uint8_t i = __builtin_ctzll(moves);
		out.insert(i | ((i - 8) << 6));
		moves &= moves - 1;
	}
	while (promote) {
		uint8_t i = __builtin_ctzll(promote);
		out.insert(i | ((i - 8) << 6) | (0b1000 << 12));
		out.insert(i | ((i - 8) << 6) | (0b1001 << 12));
		out.insert(i | ((i - 8) << 6) | (0b1010 << 12));
		out.insert(i | ((i - 8) << 6) | (0b1011 << 12));
		promote &= promote - 1;
	}
	// double moves
	moves = pawns & (~(pieces[6] | pieces[7]) << 8) & C64(0x00ff000000000000);
	moves &= ~(pieces[6] | pieces[7]) << 16;
	while (moves) {
		uint8_t i = __builtin_ctzll(moves);
		out.insert(i | ((i - 16) << 6) | (0b0001 << 12));
		moves &= moves - 1;
	}
	// captures
	while (pawns) {
		U64 pawn = pawns & -pawns;
		moves = 0;
		// take west
		moves |= (pawn & C64(0x00fefefefefefe00)) >> 9;
		// take east
		moves |= (pawn & C64(0x007f7f7f7f7f7f00)) >> 7;
		// remove the moves that are not enemy pieces
		moves &= pieces[6];
		promote = moves & C64(0x00000000000000ff);
		// remove duplicates caused by promotion
		moves &= C64(0x0000ffffffffff00);

		// en passant west
		moves |= ((pawn & C64(0x00000000fe000000)) >> 9) & BIT(meta[2]);
		// en passant east
		moves |= ((pawn & C64(0x000000007f000000)) >> 7) & BIT(meta[2]);

		pawns &= pawns - 1;

		while (moves) {
			uint8_t j = __builtin_ctzll(moves);
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b0100 << 12) | ((j == meta[2]) << 12));
			moves &= moves - 1;
		}
		while (promote) {
			uint8_t j = __builtin_ctzll(promote);
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1100 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1101 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1110 << 12));
			out.insert(__builtin_ctzll(pawn) | (j << 6) | (0b1111 << 12));
		}
	}
}

void Board::legal_moves(std::unordered_set<uint16_t> &out) {
	king_moves(out);
	// queen moves are calculated in the rook and bishop moves
	rook_moves(out);
	bishop_moves(out);
	knight_moves(out);
	if (meta[0])
		white_pawn_moves(out);
	else
		black_pawn_moves(out);
}
