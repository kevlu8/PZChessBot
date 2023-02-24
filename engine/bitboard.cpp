#include "bitboard.hpp"

Board::Board() {
	meta_hist.reserve(100);
	move_hist.reserve(100);
	memset(pieces, 0, 8 * 8);
	meta = new uint8_t[5];
	memset(meta, 0, 5);
	load_fen(STARTPOS);
}

Board::Board(const std::string &fen) {
	meta_hist.reserve(100);
	move_hist.reserve(100);
	memset(pieces, 0, 8 * 8);
	meta = new uint8_t[5];
	memset(meta, 0, 5);
	load_fen(fen);
}

void Board::load_fen(const std::string &fen) {
	if (fen == "startpos") {
		load_fen(STARTPOS);
		return;
	}
	memset(pieces, 0, 8 * 8);
	memset(meta, 0, 5);
	int currIdx = 0;
	for (int j = 7; j >= 0; j--) {
		for (int i = 0; i < 8; i++) {
			if (fen[currIdx] == '/') {
				currIdx++;
				i--;
				continue;
			}
			if (fen[currIdx] < '9') {
				i += fen[currIdx] - '1';
				currIdx++;
				continue;
			}
			if (fen[currIdx] < 'a') {
				pieces[6] |= BITxy(i, j);
				pieces[fen_to_piece[fen[currIdx] - 'B']] |= BITxy(i, j);
			} else {
				pieces[7] |= BITxy(i, j);
				pieces[fen_to_piece[fen[currIdx] - 'b']] |= BITxy(i, j);
			}
			currIdx++;
		}
	}
	currIdx++;
	meta[0] = (fen[currIdx] == 'w');
	currIdx += 2;
	if (fen[currIdx] == '-') {
		meta[1] = 0;
		currIdx++;
	} else {
		if (fen[currIdx] == 'K') {
			meta[1] ^= 0b1000;
			currIdx++;
		}
		if (fen[currIdx] == 'Q') {
			meta[1] ^= 0b0100;
			currIdx++;
		}
		if (fen[currIdx] == 'k') {
			meta[1] ^= 0b0010;
			currIdx++;
		}
		if (fen[currIdx] == 'q') {
			meta[1] ^= 0b0001;
			currIdx++;
		}
	}
	currIdx++;
	if (fen[currIdx] == '-') {
		meta[2] = -1;
		currIdx += 2;
	} else {
		meta[2] = xyx(fen[currIdx] - 'a', fen[currIdx + 1] - '1');
		currIdx += 3;
	}
	meta[3] = 0;
	while (fen[currIdx] != ' ') {
		meta[3] = meta[3] * 10 + fen[currIdx] - '0';
		currIdx++;
	}
	currIdx++;
	meta[4] = 0;
	while (currIdx < fen.size()) {
		meta[4] = meta[4] * 10 + fen[currIdx] - '0';
		currIdx++;
	}
	hash = zobrist_hash();
	pos_hist.clear();
	pos_hist[hash] = 1;
}

void Board::print_board() {
	for (int j = 7; j >= 0; j--) {
		for (int i = 0; i < 8; i++) {
			if ((pieces[6] | pieces[7]) & BITxy(i, j)) {
				int found = -1;
				bool dup = false;
				for (int k = 0; k < 6; k++) {
					if (pieces[k] & BITxy(i, j)) {
						if (found != -1)
							dup = true;
						found = k;
					}
				}
				if (dup)
					std::cout << "\U0001f595";
				else if (found != -1)
					std::cout << (char)(piece_to_fen[found] + (!!(pieces[7] & BITxy(i, j)) << 5)) << ' ';
				else
					std::cout << "? ";
			} else
				std::cout << ". ";
		}
		std::cout << '\n';
	}
}

void Board::make_move(uint16_t move) {
	uint8_t ep = 0xff;
	int src = move & 0b111111;
	int dst = (move >> 6) & 0b111111;
	// save previous board metadata
	meta_hist.push_back(meta);
	meta = new uint8_t[5];
	memcpy(meta, meta_hist.back(), 5);
	// save move for undo
	uint32_t prev = move;
	uint8_t dstpiece = -1;
	uint8_t srcpiece = -1;
	for (int i = 0; i < 6; i++) {
		if (pieces[i] & BIT(src))
			srcpiece = i;
		if (pieces[i] & BIT(dst))
			dstpiece = i;
	}
	prev |= (srcpiece << 16);
	prev |= (dstpiece << 24);
	move_hist.push_back(prev);
	if (move == 0) {
		// std::cout << "checkmate\n";
		meta[0] = !meta[0];
		hash ^= zobrist[580];
		if (pos_hist.find(hash) != pos_hist.end()) {
			pos_hist[hash]++;
		} else {
			pos_hist[hash] = 1;
		}
		return;
	}
	if (move == 0xffff) {
		// std::cout << "stalemate\n";
		meta[0] = !meta[0];
		hash ^= zobrist[580];
		if (pos_hist.find(hash) != pos_hist.end()) {
			pos_hist[hash]++;
		} else {
			pos_hist[hash] = 1;
		}
		return;
	}
	// if piece captures reset halfmove clock
	if (move & (0b0100 << 12))
		meta[3] = -1;
	if (move & 0x8000) { // promotion
		// reset halfmove clock
		meta[3] = -1;
		int piece = (move >> 12) & 0b11;
		// clear promotion square
		for (int i = 0; i < 8; i++)
			if (pieces[i] & BIT(dst)) {
				pieces[i] ^= BIT(dst);
				hash ^= zobrist[dst * 8 + i];
			}
		// remove pawn from source square
		pieces[5] &= ~BIT(src);
		hash ^= zobrist[src * 8 + 5];
		// remove pawn from player's pieces
		pieces[7 ^ meta[0]] &= ~BIT(src);
		hash ^= zobrist[src * 8 + (7 ^ meta[0])];
		// add promoted piece to destination square
		pieces[piece + 1] |= BIT(dst);
		hash ^= zobrist[dst * 8 + piece + 1];
		// add promoted piece to player's pieces
		pieces[7 ^ meta[0]] |= BIT(dst);
		hash ^= zobrist[dst * 8 + (7 ^ meta[0])];
	} else { // no promotion
		if (move >> 12 == 0b0101) { // en passant
			// reset halfmove clock
			meta[3] = -1;
			// pawn moves from src to dst
			pieces[5] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + 5];
			hash ^= zobrist[dst * 8 + 5];
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + (7 ^ meta[0])];
			hash ^= zobrist[dst * 8 + (7 ^ meta[0])];
			// remove opponent pawn
			pieces[5] ^= BIT((dst & 7) | (src & 56));
			hash ^= zobrist[((dst & 7) | (src & 56)) * 8 + 5];
			pieces[6 ^ meta[0]] ^= BIT(dst & 7 | src & 56);
			hash ^= zobrist[(dst & 7 | src & 56) * 8 + (6 ^ meta[0])];
		} else if (move == 0b0011000010000100) { // white queenside
			// king moves from e1 to c1
			pieces[0] ^= 20;
			hash ^= zobrist[32] ^ zobrist[16];
			pieces[6] ^= 20;
			hash ^= zobrist[38] ^ zobrist[22];
			// rook moves from a1 to d1
			pieces[2] ^= 9;
			hash ^= zobrist[2] ^ zobrist[26];
			pieces[6] ^= 9;
			hash ^= zobrist[6] ^ zobrist[30];
			// revoke castling rights
			if (meta[1] & 0b1000)
				hash ^= zobrist[576];
			if (meta[1] & 0b0100)
				hash ^= zobrist[577];
			meta[1] &= 0b0011;
		} else if (move == 0b0010000110000100) { // white kingside
			// king moves from e1 to g1
			pieces[0] ^= 80;
			hash ^= zobrist[32] ^ zobrist[48];
			pieces[6] ^= 80;
			hash ^= zobrist[38] ^ zobrist[54];
			// rook moves from h1 to f1
			pieces[2] ^= 160;
			hash ^= zobrist[58] ^ zobrist[42];
			pieces[6] ^= 160;
			hash ^= zobrist[62] ^ zobrist[46];
			// revoke castling rights
			if (meta[1] & 0b1000)
				hash ^= zobrist[576];
			if (meta[1] & 0b0100)
				hash ^= zobrist[577];
			meta[1] &= 0b0011;
		} else if (move == 0b0011111010111100) { // black queenside
			// king moves from e8 to c8
			pieces[0] ^= 1441151880758558720;
			hash ^= zobrist[480] ^ zobrist[464];
			pieces[7] ^= 1441151880758558720;
			hash ^= zobrist[487] ^ zobrist[471];
			// rook moves from a8 to d8
			pieces[2] ^= 648518346341351424;
			hash ^= zobrist[450] ^ zobrist[474];
			pieces[7] ^= 648518346341351424;
			hash ^= zobrist[455] ^ zobrist[479];
			// revoke castling rights
			if (meta[1] & 0b0010)
				hash ^= zobrist[578];
			if (meta[1] & 0b0001)
				hash ^= zobrist[579];
			meta[1] &= 0b1100;
		} else if (move == 0b0010111110111100) { // black kingside
			// king moves from e8 to g8
			pieces[0] ^= 5764607523034234880;
			hash ^= zobrist[480] ^ zobrist[496];
			pieces[7] ^= 5764607523034234880;
			hash ^= zobrist[487] ^ zobrist[503];
			// rook moves from h8 to f8
			pieces[2] ^= 11529215046068469760;
			hash ^= zobrist[506] ^ zobrist[490];
			pieces[7] ^= 11529215046068469760;
			hash ^= zobrist[511] ^ zobrist[495];
			// revoke castling rights
			if (meta[1] & 0b0010)
				hash ^= zobrist[578];
			if (meta[1] & 0b0001)
				hash ^= zobrist[579];
			meta[1] &= 0b1100;
		} else if (pieces[0] & BIT(src)) { // king move
			// remove castling rights
			if (meta[0]) {
				if (meta[1] & 0b1000)
					hash ^= zobrist[576];
				if (meta[1] & 0b0100)
					hash ^= zobrist[577];
				meta[1] &= 0b0011;
			} else {
				if (meta[1] & 0b0010)
					hash ^= zobrist[578];
				if (meta[1] & 0b0001)
					hash ^= zobrist[579];
				meta[1] &= 0b1100;
			}
			// clear destination square
			for (int i = 0; i < 8; i++)
				if (pieces[i] & BIT(dst)) {
					pieces[i] ^= BIT(dst);
					hash ^= zobrist[dst * 8 + i];
				}
			// move king
			pieces[0] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8] ^ zobrist[dst * 8];
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + (7 ^ meta[0])] ^ zobrist[dst * 8 + (7 ^ meta[0])];
		} else {
			// rooks moves or is taken = no castle
			if (src == 0 || dst == 0) { // white queen rook
				if (meta[1] & 0b0100)
					hash ^= zobrist[577];
				meta[1] &= 0b1011;
			} else if (src == 7 || dst == 7) { // white king rook
				if (meta[1] & 0b1000)
					hash ^= zobrist[576];
				meta[1] &= 0b0111;
			} else if (src == 56 || dst == 56) { // black queen rook
				if (meta[1] & 0b0001)
					hash ^= zobrist[579];
				meta[1] &= 0b1110;
			} else if (src == 63 || dst == 63) { // black king rook
				if (meta[1] & 0b0010)
					hash ^= zobrist[578];
				meta[1] &= 0b1101;
			}
			// pawn move = reset halfmove clock
			if (pieces[5] & BIT(src)) {
				meta[3] = -1;
				if (pieces[5] & BIT(src) && abs(src - dst) == 16) { // double pawn move
					// set en passant square
					ep = (src + dst) >> 1;
				}
			}
			// clear destination square
			for (int i = 0; i < 8; i++)
				if (pieces[i] & BIT(dst)) {
					pieces[i] ^= BIT(dst);
					hash ^= zobrist[dst * 8 + i];
				}
			// figure out what piece is moving
			int piece = (prev >> 16) & 0b111;
			// piece moves from src to dst
			pieces[piece] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + piece] ^ zobrist[dst * 8 + piece];
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + (7 ^ meta[0])] ^ zobrist[dst * 8 + (7 ^ meta[0])];
		}
	}
	meta[0] = !meta[0];
	hash ^= zobrist[580];
	meta[3]++;
	meta[4] += meta[0];
	if (ep == 0xff) {
		if (meta[2] != ep) {
			hash ^= zobrist[meta[2] + 512];
		}
	} else {
		if (meta[2] != 0xff)
			hash ^= zobrist[meta[2] + 512];
		hash ^= zobrist[ep + 512];
	}
	meta[2] = ep;
	// update position history
	if (pos_hist.find(hash) != pos_hist.end()) {
		pos_hist[hash]++;
	} else {
		pos_hist[hash] = 1;
	}
}

void Board::unmake_move() {
	// update position history
	if (pos_hist[hash] <= 1) {
		pos_hist.erase(hash);
	} else {
		pos_hist[hash]--;
	}
	uint32_t prev = move_hist.back();
	// restore previous metadata
	uint8_t *oldmeta = meta;
	meta = meta_hist.back();
	meta_hist.pop_back();
	if (oldmeta[0] != meta[0])
		hash ^= zobrist[580];
	if ((oldmeta[1] ^ meta[1]) & 0b1000)
		hash ^= zobrist[576];
	if ((oldmeta[1] ^ meta[1]) & 0b0100)
		hash ^= zobrist[577];
	if ((oldmeta[1] ^ meta[1]) & 0b0010)
		hash ^= zobrist[578];
	if ((oldmeta[1] ^ meta[1]) & 0b0001)
		hash ^= zobrist[579];
	if (oldmeta[2] != meta[2]) {
		if (meta[2] == 0xff)
			hash ^= zobrist[oldmeta[2] + 512];
		else if (oldmeta[2] == 0xff)
			hash ^= zobrist[meta[2] + 512];
		else
			hash ^= zobrist[oldmeta[2] + 512] ^ zobrist[meta[2] + 512];
	}
	delete[] oldmeta;
	// get previous move
	move_hist.pop_back();
	uint8_t src = prev & 0b111111;
	uint8_t dst = (prev >> 6) & 0b111111;
	uint16_t move = prev & 0xffff;
	if (move == 0 || move == 0xffff) {
		return;
	}
	if (prev & 0x8000) { // promotion
		// remove promoted piece
		pieces[((prev >> 12) & 0b11) + 1] ^= BIT(dst);
		hash ^= zobrist[dst * 8 + ((prev >> 12) & 0b11) + 1];
		pieces[7 ^ meta[0]] ^= BIT(dst);
		hash ^= zobrist[dst * (7 ^ meta[0])];
		// put the pawn back
		pieces[5] ^= BIT(src);
		hash ^= zobrist[src * 8 + 5];
		pieces[7 ^ meta[0]] ^= BIT(src);
		hash ^= zobrist[src * 8 + (7 ^ meta[0])];
		// put the captured piece back
		if (prev >> 24 != 0xff) {
			pieces[prev >> 24] ^= BIT(dst);
			hash ^= zobrist[dst * 8 + (prev >> 24)];
			pieces[6 ^ meta[0]] ^= BIT(dst);
			hash ^= zobrist[dst * 8 + (6 ^ meta[0])];
		}
	} else { // no promotion
		if (((prev >> 12) & 0b1111) == 0b0101) { // en passant
			// put the pawn back
			pieces[5] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + 5] ^ zobrist[dst * 8 + 5];
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + (7 ^ meta[0])] ^ zobrist[dst * 8 + (7 ^ meta[0])];
			// put the captured pawn back
			pieces[5] ^= BIT(dst & 7 | src & 56);
			hash ^= zobrist[(dst & 7 | src & 56) * 8 + 5];
			pieces[6 ^ meta[0]] ^= BIT(dst & 7 | src & 56);
			hash ^= zobrist[(dst & 7 | src & 56) * 8 + (6 ^ meta[0])];
		} else if (move == 0b0011000010000100) { // white queenside
			// king moves from c1 to e1
			pieces[0] ^= 20;
			hash ^= zobrist[32] ^ zobrist[16];
			pieces[6] ^= 20;
			hash ^= zobrist[38] ^ zobrist[22];
			// rook moves from d1 to a1
			pieces[2] ^= 9;
			hash ^= zobrist[2] ^ zobrist[26];
			pieces[6] ^= 9;
			hash ^= zobrist[6] ^ zobrist[30];
		} else if (move == 0b0010000110000100) { // white kingside
			// king moves from g1 to e1
			pieces[0] ^= 80;
			hash ^= zobrist[32] ^ zobrist[48];
			pieces[6] ^= 80;
			hash ^= zobrist[38] ^ zobrist[54];
			// rook moves from f1 to h1
			pieces[2] ^= 160;
			hash ^= zobrist[58] ^ zobrist[42];
			pieces[6] ^= 160;
			hash ^= zobrist[62] ^ zobrist[46];
		} else if (move == 0b0011111010111100) { // black queenside
			// king moves from c8 to e8
			pieces[0] ^= 1441151880758558720;
			hash ^= zobrist[480] ^ zobrist[464];
			pieces[7] ^= 1441151880758558720;
			hash ^= zobrist[487] ^ zobrist[471];
			// rook moves from d8 to a8
			pieces[2] ^= 648518346341351424;
			hash ^= zobrist[450] ^ zobrist[474];
			pieces[7] ^= 648518346341351424;
			hash ^= zobrist[455] ^ zobrist[479];
		} else if (move == 0b0010111110111100) { // black kingside
			// king moves from g8 to e8
			pieces[0] ^= 5764607523034234880;
			hash ^= zobrist[480] ^ zobrist[496];
			pieces[7] ^= 5764607523034234880;
			hash ^= zobrist[487] ^ zobrist[503];
			// rook moves from f8 to h8
			pieces[2] ^= 11529215046068469760;
			hash ^= zobrist[506] ^ zobrist[490];
			pieces[7] ^= 11529215046068469760;
			hash ^= zobrist[511] ^ zobrist[495];
		} else {
			// figure out what piece moved
			int piece = (prev >> 16) & 0b111;
			// move piece back
			pieces[piece] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + piece] ^ zobrist[dst * 8 + piece];
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			hash ^= zobrist[src * 8 + (7 ^ meta[0])] ^ zobrist[dst * 8 + (7 ^ meta[0])];
			// replace the captured piece
			if (prev >> 24 != 0xff) {
				pieces[prev >> 24] ^= BIT(dst);
				hash ^= zobrist[dst * 8 + (prev >> 24)];
				pieces[6 ^ meta[0]] ^= BIT(dst);
				hash ^= zobrist[dst * 8 + (6 ^ meta[0])];
			}
		}
	}
}

bool Board::in_check(const bool side) { return pieces[0] & pieces[7 ^ side] & control[!side]; }

U64 Board::zobrist_hash() {
	U64 hash = 0;
	for (int i = 0; i < 8; i++) {
		U64 p = pieces[i];
		while (p) {
			int sq = __builtin_ctzll(p);
			hash ^= zobrist[sq * 8 + i];
			p &= p - 1;
		}
	}
	if (meta[1] & 0b1000)
		hash ^= zobrist[576];
	if (meta[1] & 0b0100)
		hash ^= zobrist[577];
	if (meta[1] & 0b0010)
		hash ^= zobrist[578];
	if (meta[1] & 0b0001)
		hash ^= zobrist[579];
	if (meta[0])
		hash ^= zobrist[580];
	if (meta[2] != 0xff)
		hash ^= zobrist[meta[2] + 512];
	return hash;
}

std::string stringify_move(uint16_t move) {
	if (move == 0)
		return "0000";
	if (move == 0xffff)
		return "----";
	std::string str = "";
	str += (char)('a' + (move & 7));
	str += (char)('1' + ((move >> 3) & 7));
	str += (char)('a' + ((move >> 6) & 7));
	str += (char)('1' + ((move >> 9) & 7));
	if (move & 0b1000000000000000) { // promotion
		str += piece_to_fen[((move >> 12) & 0b11) + 1] + 32;
	}
	return str;
}

uint16_t parse_move(Board &b, std::string str) {
	std::cout << "parsing move: " << str << std::endl;
	if (str == "e1c1" && (b.kings() & BIT(4)))
		return 0b0011000010000100;
	if (str == "e1g1" && (b.kings() & BIT(4)))
		return 0b0010000110000100;
	if (str == "e8c8" && (b.kings() & BIT(60)))
		return 0b0011111010111100;
	if (str == "e8g8" && (b.kings() & BIT(60)))
		return 0b0010111110111100;
	if (str == "0000")
		return 0;
	if (str == "----")
		return 0;
	uint16_t move = 0;
	char src = str[0] - 'a' + (str[1] - '1') * 8;
	char dst = str[2] - 'a' + (str[3] - '1') * 8;
	if (b.occupied() & BIT(dst)) // set capture flag if capture
		move |= 0b0100 << 12;
	if (str.size() == 5) { // promotion
		move |= 0b1000 << 12;
		move |= (fen_to_piece[str[4] - 'b'] - 1) << 12;
	}
	if (b.pawns() & BIT(src)) { // pawn move
		if (abs(src - dst) == 16) // set double push flag if double push
			move |= 0b0001 << 12;
		if ((src & 0b111) != (dst & 0b111) && !(b.occupied() & BIT(dst))) { // set en passant flag if en passant
			// if (b.pawns() & BIT(dst & 7 | src & 56))
			move |= 0b0101 << 12;
		}
	}
	return move | src | (dst << 6);
}
