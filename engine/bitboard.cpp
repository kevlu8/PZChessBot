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
	if (move == 0) {
		std::cout << "null move\n";
		return;
	}
	for (int i = 0; i < 8; i++)
		pos_hist.push_back(pieces[i]);
	uint8_t ep = -1;
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
	// if piece captures reset halfmove clock
	if (move & (0b0100 << 12))
		meta[3] = -1;
	if (move & 0x8000) { // promotion
		// reset halfmove clock
		meta[3] = -1;
		int piece = (move >> 12) & 0b11;
		// clear promotion square
		for (int i = 0; i < 8; i++)
			pieces[i] &= ~BIT(dst);
		// remove pawn from source square
		pieces[5] &= ~BIT(src);
		// remove pawn from player's pieces
		pieces[7 ^ meta[0]] &= ~BIT(src);
		// add promoted piece to destination square
		pieces[piece + 1] |= BIT(dst);
		// add promoted piece to player's pieces
		pieces[7 ^ meta[0]] |= BIT(dst);
	} else { // no promotion
		if (move >> 12 == 0b0101) { // en passant
			// reset halfmove clock
			meta[3] = -1;
			// pawn moves from src to dst
			pieces[5] ^= BIT(src) | BIT(dst);
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			// remove opponent pawn
			pieces[5] ^= BIT(dst & 7 | src & 56);
			pieces[6 ^ meta[0]] ^= BIT(dst & 7 | src & 56);
		} else if (move == 0b0011000010000100) { // white queenside
			// king moves from e1 to c1
			pieces[0] ^= 20;
			pieces[6] ^= 20;
			// rook moves from a1 to d1
			pieces[2] ^= 9;
			pieces[6] ^= 9;
			// revoke castling rights
			meta[1] &= 0b0011;
		} else if (move == 0b0010000110000100) { // white kingside
			// king moves from e1 to g1
			pieces[0] ^= 80;
			pieces[6] ^= 80;
			// rook moves from h1 to f1
			pieces[2] ^= 160;
			pieces[6] ^= 160;
			// revoke castling rights
			meta[1] &= 0b0011;
		} else if (move == 0b0011111010111100) { // black queenside
			// king moves from e8 to c8
			pieces[0] ^= 1441151880758558720;
			pieces[7] ^= 1441151880758558720;
			// rook moves from a8 to d8
			pieces[2] ^= 648518346341351424;
			pieces[7] ^= 648518346341351424;
			// revoke castling rights
			meta[1] &= 0b1100;
		} else if (move == 0b0010111110111100) { // black kingside
			// king moves from e8 to g8
			pieces[0] ^= 5764607523034234880;
			pieces[7] ^= 5764607523034234880;
			// rook moves from h8 to f8
			pieces[2] ^= 11529215046068469760;
			pieces[7] ^= 11529215046068469760;
			// revoke castling rights
			meta[1] &= 0b1100;
		} else if (pieces[0] & BIT(src)) { // king move
			// remove castling rights
			meta[1] &= 0b1100 >> (meta[0] * 2);
			// clear destination square
			for (int i = 0; i < 8; i++)
				pieces[i] &= ~BIT(dst);
			pieces[0] ^= BIT(src) | BIT(dst);
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
		} else {
			// rooks moves or is taken = no castle
			if (src == 0 || dst == 0) { // white queen rook
				meta[1] &= 0b1011;
			} else if (src == 7 || dst == 7) { // white king rook
				meta[1] &= 0b0111;
			} else if (src == 56 || dst == 56) { // black queen rook
				meta[1] &= 0b1110;
			} else if (src == 63 || dst == 63) { // black king rook
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
				pieces[i] &= ~BIT(dst);
			// figure out what piece is moving
			int piece = (prev >> 16) & 0b111;
			// piece moves from src to dst
			pieces[piece] ^= BIT(src) | BIT(dst);
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
		}
	}
	meta[0] = !meta[0];
	meta[3]++;
	meta[4] += meta[0];
	meta[2] = ep;
}

void Board::unmake_move() {
	// restore previous metadata
	delete[] meta;
	meta = meta_hist.back();
	meta_hist.pop_back();
	// get previous move
	uint32_t prev = move_hist.back();
	move_hist.pop_back();
	uint8_t src = prev & 0b111111;
	uint8_t dst = (prev >> 6) & 0b111111;
	uint16_t move = prev & 0xffff;
	if (prev & 0x8000) { // promotion
		// remove promoted piece
		pieces[((prev >> 12) & 0b11) + 1] ^= BIT(dst);
		pieces[7 ^ meta[0]] ^= BIT(dst);
		// put the pawn back
		pieces[5] ^= BIT(src);
		pieces[7 ^ meta[0]] ^= BIT(src);
		// put the captured piece back
		if (prev >> 24 != 0xff) {
			pieces[prev >> 24] ^= BIT(dst);
			pieces[6 ^ meta[0]] ^= BIT(dst);
		}
	} else { // no promotion
		if (((prev >> 12) & 0b1111) == 0b0101) { // en passant
			// put the pawn back
			pieces[5] ^= BIT(src) | BIT(dst);
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			// put the captured pawn back
			pieces[5] ^= BIT(dst & 7 | src & 56);
			pieces[6 ^ meta[0]] ^= BIT(dst & 7 | src & 56);
		} else if (move == 0b0011000010000100) { // white queenside
			// king moves from c1 to e1
			pieces[0] ^= 20;
			pieces[6] ^= 20;
			// rook moves from d1 to ai
			pieces[2] ^= 9;
			pieces[6] ^= 9;
		} else if (move == 0b0010000110000100) { // white kingside
			// king moves from g1 to e1
			pieces[0] ^= 80;
			pieces[6] ^= 80;
			// rook moves from f1 to h1
			pieces[2] ^= 160;
			pieces[6] ^= 160;
		} else if (move == 0b0011111010111100) { // black queenside
			// king moves from c8 to e8
			pieces[0] ^= 1441151880758558720;
			pieces[7] ^= 1441151880758558720;
			// rook moves from d8 to a8
			pieces[2] ^= 648518346341351424;
			pieces[7] ^= 648518346341351424;
		} else if (move == 0b0010111110111100) { // black kingside
			// king moves from g8 to e8
			pieces[0] ^= 5764607523034234880;
			pieces[7] ^= 5764607523034234880;
			// rook moves from f8 to h8
			pieces[2] ^= 11529215046068469760;
			pieces[7] ^= 11529215046068469760;
		} else {
			// figure out what piece moved
			int piece = (prev >> 16) & 0b111;
			// move piece back
			pieces[piece] ^= BIT(src) | BIT(dst);
			pieces[7 ^ meta[0]] ^= BIT(src) | BIT(dst);
			// replace the captured piece
			if (prev >> 24 != 0xff) {
				pieces[prev >> 24] ^= BIT(dst);
				pieces[6 ^ meta[0]] ^= BIT(dst);
			}
		}
	}
	for (int i = 7; i >= 0; i--) {
		if (pos_hist.back() != pieces[i])
			throw std::runtime_error("position mismatch");
		pos_hist.pop_back();
	}
}

bool Board::in_check(const bool side) { return pieces[0] & pieces[7 ^ side] & control[!side]; }

U64 Board::zobrist_hash() {
	U64 hash = 0;
	return hash;
}

std::string stringify_move(uint16_t move) {
	if (move == 0)
		return "0000";
	if (move == -1)
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
