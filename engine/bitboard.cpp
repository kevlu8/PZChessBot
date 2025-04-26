#include "bitboard.hpp"
#include <cctype>
#include <random>

uint64_t zobrist_square[64][15];
uint64_t zobrist_castling[16];
uint64_t zobrist_ep[9];
uint64_t zobrist_side;

__attribute__((constructor)) void init_zobrist() {
	std::mt19937_64 rng(0xdeadbeef);
	std::uniform_int_distribution<uint64_t> dist;

	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 14; j++) {
			zobrist_square[i][j] = dist(rng);
		}
		zobrist_square[i][14] = 0;
	}

	for (int i = 0; i < 16; i++) {
		zobrist_castling[i] = dist(rng);
	}

	for (int i = 0; i < 8; i++) {
		zobrist_ep[i] = dist(rng);
	}
	zobrist_ep[8] = 0;

	zobrist_side = dist(rng);
}

void print_bitboard(Bitboard board) {
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			std::cout << (((board >> (i * 8 + j)) & 1) ? "X " : ". ");
		}
		std::cout << '\n';
	}
}

std::string Move::to_string() const {
	if (data == 0)
		return "0000";
	std::string str = "";
	str += (char)('a' + (src() & 0b111));
	str += (char)('1' + (src() >> 3));
	str += (char)('a' + (dst() & 0b111));
	str += (char)('1' + (dst() >> 3));
	if ((data & 0xc000) == PROMOTION) {
		str += piecetype_letter[((data >> 12) & 0b11) + KNIGHT];
	}
	return str;
}

Move Move::from_string(const std::string &str, const void *b) {
	if (str == "0000")
		return NullMove;
	int src_file = str[0] - 'a';
	int src_rank = str[1] - '1';
	int dst_file = str[2] - 'a';
	int dst_rank = str[3] - '1';
	int src = src_rank * 8 + src_file;
	int dst = dst_rank * 8 + dst_file;

	if (str.size() == 5) {
		// Promotion move
		char promo = std::tolower(str[4]);
		PieceType pt;
		switch (promo) {
		case 'n':
			pt = KNIGHT;
			break;
		case 'b':
			pt = BISHOP;
			break;
		case 'r':
			pt = ROOK;
			break;
		case 'q':
			pt = QUEEN;
			break;
		default:
			pt = QUEEN;
			break; // default to queen
		}
		return Move::make<PROMOTION>(src, dst, pt);
	} else {
		if ((((const Board *)b)->mailbox[src] & 7) == KING) {
			// Check for castling
			if (str == "e1g1" || str == "e8g8" || str == "e1c1" || str == "e8c8") {
				return Move::make<CASTLING>(src, dst);
			}
		} else if ((((const Board *)b)->mailbox[src] & 7) == PAWN && dst == ((const Board *)b)->ep_square) {
			// En passant
			return Move::make<EN_PASSANT>(src, dst);
		}
		return Move(src, dst);
	}
}

void Board::load_fen(std::string fen) {
	if (fen == "startpos")
		fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

	memset(piece_boards, 0, sizeof(piece_boards));
	memset(mailbox, NO_PIECE, sizeof(mailbox));
	castling = NO_CASTLE;
	uint16_t rank = RANK_8;
	uint16_t file = FILE_A;
	int inputIdx = 0;
	// Load piece data
	while (rank <= RANK_8) { // Will catch wrap-around subtractions
		char cur = fen[inputIdx++];
		if (cur == '/') // Ignore slashes
			continue;
		if (cur <= '9') { // Character is a number indicating blank squares
			// Skip the required amount of squares
			file += cur - '0';
		} else { // Must be a piece
			// We ignore the first first part of the ASCII representation
			PieceType piece = letter_piece[(cur & 0x1f) - 2];
			piece_boards[piece] |= square_bits((Rank)rank, (File)file);
			// Uppercase in ASCII is 0b010xxxxx, lowercase is 0b011xxxxx
			// Chooses either [6] or [7] (white or black occupancy)
			piece_boards[(6 ^ 0b10) ^ (cur >> 5)] |= square_bits((Rank)rank, (File)file);
			mailbox[rank * 8 + file] = Piece(piece + ((cur >> 2) & 0b1000));
			file++;
		}
		// Fix overflow
		rank -= file >> 3;
		file &= 7;
	}
	// Set side according to FEN data ('w' (odd) for white, 'b' (even) for black)
	side = (~fen[inputIdx + 1]) & 1;
	inputIdx += 3;

	// Load castling rights
	if (fen[inputIdx] != '-') {
		if (fen[inputIdx] == 'K') {
			castling |= WHITE_OO;
			inputIdx++;
		}
		if (fen[inputIdx] == 'Q') {
			castling |= WHITE_OOO;
			inputIdx++;
		}
		if (fen[inputIdx] == 'k') {
			castling |= BLACK_OO;
			inputIdx++;
		}
		if (fen[inputIdx] == 'q') {
			castling |= BLACK_OOO;
			inputIdx++;
		}
		inputIdx++;
	} else {
		inputIdx += 2;
	}

	// Load EP square
	if (fen[inputIdx] != '-') {
		ep_square = Square((fen[inputIdx] - 'a') + 8 * (fen[inputIdx + 1] - '1'));
		inputIdx += 3;
	} else {
		inputIdx += 2;
	}

	// Get halfmove clock
	halfmove = 0;
	while (inputIdx < fen.size() && std::isdigit(fen[inputIdx])) {
		halfmove *= 10;
		halfmove += fen[inputIdx] - '0';
		inputIdx++;
	}

	// Ignore the rest (who cares anyways)

	// Recompute hash
	recompute_hash();
}

std::string Board::get_fen() const {
	std::string res = "";
	for (int rank = RANK_8; rank >= 0; rank--) {
		int empty = 0;
		for (int file = FILE_A; file <= FILE_H; file++) {
			if (mailbox[rank * 8 + file] == NO_PIECE) {
				empty++;
			} else {
				if (empty > 0) {
					res += std::to_string(empty);
					empty = 0;
				}
				res += piece_letter[mailbox[rank * 8 + file]];
			}
		}
		if (empty > 0) {
			res += std::to_string(empty);
		}
		if (rank > 0) {
			res += '/';
		}
	}
	res += side ? " b " : " w ";
	if (castling == NO_CASTLE) {
		res += "- ";
	} else {
		if (castling & WHITE_OO)
			res += "K";
		if (castling & WHITE_OOO)
			res += "Q";
		if (castling & BLACK_OO)
			res += "k";
		if (castling & BLACK_OOO)
			res += "q";
	}
	return res; // Shortened fen (no ep, halfmove, etc) since we don't need those in datagen
}

bool Board::sanity_check(char *print) {
	bool error = 0;
	// Start at -1 because we increment before processing to guarantee it happens in every case
	int printIdx = -1;
	// Occupancy
	Bitboard occ = piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)];
	// Used for sanity checks during debugging (piece set but no occupancy)
	Bitboard sanity = (piece_boards[0] | piece_boards[1] | piece_boards[2] | piece_boards[3] | piece_boards[4] | piece_boards[5]) ^ occ;
	for (int rank = RANK_8; rank >= 0; rank--) {
		for (int file = FILE_A; file <= FILE_H; file++) {
			Bitboard bits = square_bits((Rank)rank, (File)file);
			printIdx++;
			if (mailbox[rank * 8 + file] != NO_PIECE && !(occ & square_bits((Rank)rank, (File)file))) { // Occupancy and mailbox differ
				print[printIdx] = '$';
				std::cerr << "Occupancy and mailbox differ on square " << rank << file << '\n';
				error = 1;
				continue;
			}
			if (sanity & bits) { // Occupancy and collective piece boards differ
				if (occ & bits) { // Occupied but no piece specified
					print[printIdx] = '?';
					std::cerr << "Occupied but no piece specified on square " << rank << file << '\n';
					error = 1;
				} else { // Piece specified but no occupancy
					print[printIdx] = '!';
					std::cerr << "Piece specified but no occupancy on square " << rank << file << '\n';
					error = 1;
				}
				continue;
			}
			if (!(occ & bits)) {
				print[printIdx] = '.';
				continue;
			}

			bool put = false;
			// Scan through piece types trying to find one that matches
			for (int type = PAWN; type < NO_PIECETYPE; type++) {
				if (!(piece_boards[type] & bits))
					continue;
				if (put) { // More than two pieces on one square
					print[printIdx] = '&';
					std::cerr << "More than two pieces on square " << rank << file << '\n';
					error = 1;
					break;
				}
				put = true;
				if (mailbox[rank * 8 + file] != type + (!!(piece_boards[OCC(BLACK)] & bits) << 3)) { // Bitboard and mailbox representations differ
					print[printIdx] = '$';
					std::cerr << "Bitboard and mailbox representations differ on square " << rank << file << '\n';
					error = 1;
					break;
				}
				print[printIdx] = piecetype_letter[type] + (!!(piece_boards[OCC(BLACK)] & bits) << 5);
			}
		}
	}

	return error;
}

void Board::print_board() const {
	if (castling == NO_CASTLE) {
		std::cout << "-";
	} else {
		if (castling & WHITE_OO)
			std::cout << "K";
		if (castling & WHITE_OOO)
			std::cout << "Q";
		if (castling & BLACK_OO)
			std::cout << "k";
		if (castling & BLACK_OOO)
			std::cout << "q";
	}

	if (ep_square == SQ_NONE)
		std::cout << " - ";
	else
		std::cout << ' ' << (char)('a' + (ep_square & 0b111)) << (char)('1' + (ep_square >> 3)) << ' ';
	std::cout << (side ? "black" : "white") << '\n';

#ifndef NODEBUG
	char print[64];
	// Start at -1 because we increment before processing to guarantee it happens in every case
	int printIdx = -1;
	// Occupancy
	Bitboard occ = piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)];
	// Used for sanity checks during debugging (piece set but no occupancy)
	Bitboard sanity = (piece_boards[0] | piece_boards[1] | piece_boards[2] | piece_boards[3] | piece_boards[4] | piece_boards[5]) ^ occ;
	for (int rank = RANK_8; rank >= 0; rank--) {
		for (int file = FILE_A; file <= FILE_H; file++) {
			Bitboard bits = square_bits((Rank)rank, (File)file);
			printIdx++;
			if (mailbox[rank * 8 + file] != NO_PIECE && !(occ & square_bits((Rank)rank, (File)file))) { // Occupancy and mailbox differ
				print[printIdx] = '$';
				continue;
			}
			if (sanity & bits) { // Occupancy and collective piece boards differ
				if (occ & bits) // Occupied but no piece specified
					print[printIdx] = '?';
				else // Piece specified but no occupancy
					print[printIdx] = '!';
				continue;
			}
			if (!(occ & bits)) {
				print[printIdx] = '.';
				continue;
			}

			bool put = false;
			// Scan through piece types trying to find one that matches
			for (int type = PAWN; type < NO_PIECETYPE; type++) {
				if (!(piece_boards[type] & bits))
					continue;
				if (put) { // More than two pieces on one square
					print[printIdx] = '&';
					break;
				}
				put = true;
				if (mailbox[rank * 8 + file] != type + (!!(piece_boards[OCC(BLACK)] & bits) << 3)) { // Bitboard and mailbox representations differ
					print[printIdx] = '$';
					break;
				}
				print[printIdx] = piece_letter[mailbox[rank * 8 + file]];
			}
		}
	}
	// Print the board
	for (int i = 0; i < 64; i++) {
		std::cout << print[i];
		if (i % 8 == 7)
			std::cout << '\n';
		else
			std::cout << ' ';
	}
#else
	for (int rank = RANK_8; rank >= 0; rank--) {
		for (int file = FILE_A; file <= FILE_H; file++) {
			std::cout << piece_letter[mailbox[rank * 8 + file]];
			if (file == FILE_H)
				std::cout << '\n';
			else
				std::cout << ' ';
		}
	}
#endif
}

void Board::make_move(Move move) {
#ifdef SANCHECK
	char before[64];
	sanity_check(before);
#endif

#ifdef HASHCHECK
	uint64_t old_hash = zobrist;
	recompute_hash();
	if (old_hash != zobrist) {
		std::cerr << "Hash mismatch before move: expected " << zobrist << " got " << old_hash << '\n';
		abort();
	}
#endif

	// Add move to move history
	move_hist.push(HistoryEntry(move, mailbox[move.dst()], castling, ep_square));
	halfmove_hist.push(halfmove);
	Square tmp_ep_square = SQ_NONE;

	// Handle captures
	if (move.data != 0 && (piece_boards[OPPOCC(side)] & square_bits(move.dst()))) { // If opposite occupancy bit set on destination (capture)
		// Remove whatever piece it was
		uint8_t piece = mailbox[move.dst()] & 0b111;
		piece_boards[piece] ^= square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(move.dst());
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]];

		if (piece == ROOK) {
			uint8_t old_castling = castling;
			if (move.dst() == SQ_A1)
				castling &= ~WHITE_OOO;
			else if (move.dst() == SQ_H1)
				castling &= ~WHITE_OO;
			else if (move.dst() == SQ_A8)
				castling &= ~BLACK_OOO;
			else if (move.dst() == SQ_H8)
				castling &= ~BLACK_OO;
			zobrist ^= zobrist_castling[old_castling] ^ zobrist_castling[castling];
		}

		halfmove = -1;
	}

	if ((mailbox[move.src()] & 0b111) == PAWN)
		halfmove = -1;

	if (move.data == 0) {
		// Null move, do nothing, just change sides
	} else if (move.type() == PROMOTION) {
		// Remove the pawn on the src and add the piece on the dst
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]];
		mailbox[move.src()] = NO_PIECE;
		mailbox[move.dst()] = Piece(move.promotion() + ((!!side) << 3) + KNIGHT);
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]];
		piece_boards[PAWN] ^= square_bits(move.src());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[move.promotion() + KNIGHT] ^= square_bits(move.dst());
	} else if (move.type() == EN_PASSANT) {
		// Remove the pawn on the src and the taken pawn, then add the pawn on the dst
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		zobrist ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]]; // Taken pawn
		mailbox[move.dst()] = mailbox[move.src()];
		mailbox[move.src()] = NO_PIECE;
		mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)] = NO_PIECE;
		piece_boards[PAWN] ^= square_bits(move.src()) | square_bits(move.dst()) | square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
	} else if (move.type() == CASTLING) {
		// Calculate where the rook is
		Bitboard rook_mask;
		if (move.data == 0b1100000100000110) {
			// White O-O
			mailbox[SQ_E1] = NO_PIECE;
			mailbox[SQ_G1] = Piece(WHITE_KING);
			mailbox[SQ_H1] = NO_PIECE;
			mailbox[SQ_F1] = Piece(WHITE_ROOK);
			piece_boards[OCC(WHITE)] ^= square_bits(SQ_E1) | square_bits(SQ_G1) | square_bits(SQ_H1) | square_bits(SQ_F1);
			piece_boards[KING] ^= square_bits(SQ_E1) | square_bits(SQ_G1);
			piece_boards[ROOK] ^= square_bits(SQ_H1) | square_bits(SQ_F1);
			zobrist ^= zobrist_square[SQ_E1][Piece(WHITE_KING)] ^ zobrist_square[SQ_G1][Piece(WHITE_KING)];
			zobrist ^= zobrist_square[SQ_H1][Piece(WHITE_ROOK)] ^ zobrist_square[SQ_F1][Piece(WHITE_ROOK)];
		} else if (move.data == 0b1100000100000010) {
			// White O-O-O
			mailbox[SQ_E1] = NO_PIECE;
			mailbox[SQ_C1] = Piece(WHITE_KING);
			mailbox[SQ_A1] = NO_PIECE;
			mailbox[SQ_D1] = Piece(WHITE_ROOK);
			piece_boards[OCC(WHITE)] ^= square_bits(SQ_E1) | square_bits(SQ_C1) | square_bits(SQ_A1) | square_bits(SQ_D1);
			piece_boards[KING] ^= square_bits(SQ_E1) | square_bits(SQ_C1);
			piece_boards[ROOK] ^= square_bits(SQ_A1) | square_bits(SQ_D1);
			zobrist ^= zobrist_square[SQ_E1][Piece(WHITE_KING)] ^ zobrist_square[SQ_C1][Piece(WHITE_KING)];
			zobrist ^= zobrist_square[SQ_A1][Piece(WHITE_ROOK)] ^ zobrist_square[SQ_D1][Piece(WHITE_ROOK)];
		} else if (move.data == 0b1100111100111110) {
			// Black O-O
			mailbox[SQ_E8] = NO_PIECE;
			mailbox[SQ_G8] = Piece(BLACK_KING);
			mailbox[SQ_H8] = NO_PIECE;
			mailbox[SQ_F8] = Piece(BLACK_ROOK);
			piece_boards[OCC(BLACK)] ^= square_bits(SQ_E8) | square_bits(SQ_G8) | square_bits(SQ_H8) | square_bits(SQ_F8);
			piece_boards[KING] ^= square_bits(SQ_E8) | square_bits(SQ_G8);
			piece_boards[ROOK] ^= square_bits(SQ_H8) | square_bits(SQ_F8);
			zobrist ^= zobrist_square[SQ_E8][Piece(BLACK_KING)] ^ zobrist_square[SQ_G8][Piece(BLACK_KING)];
			zobrist ^= zobrist_square[SQ_H8][Piece(BLACK_ROOK)] ^ zobrist_square[SQ_F8][Piece(BLACK_ROOK)];
		} else if (move.data == 0b1100111100111010) {
			// Black O-O-O
			mailbox[SQ_E8] = NO_PIECE;
			mailbox[SQ_C8] = Piece(BLACK_KING);
			mailbox[SQ_A8] = NO_PIECE;
			mailbox[SQ_D8] = Piece(BLACK_ROOK);
			piece_boards[OCC(BLACK)] ^= square_bits(SQ_E8) | square_bits(SQ_C8) | square_bits(SQ_A8) | square_bits(SQ_D8);
			piece_boards[KING] ^= square_bits(SQ_E8) | square_bits(SQ_C8);
			piece_boards[ROOK] ^= square_bits(SQ_A8) | square_bits(SQ_D8);
			zobrist ^= zobrist_square[SQ_E8][Piece(BLACK_KING)] ^ zobrist_square[SQ_C8][Piece(BLACK_KING)];
			zobrist ^= zobrist_square[SQ_A8][Piece(BLACK_ROOK)] ^ zobrist_square[SQ_D8][Piece(BLACK_ROOK)];
		} else {
			std::cerr << "Il faut que tu meures" << std::endl;
			volatile int *p = 0;
			*p = move.type();
		}
		// Remove castling rights
		castling &= ~((WHITE_OO | WHITE_OOO) << (side + side));
	} else {
		// Get piece that is moving
		uint8_t piece = mailbox[move.src()] & 0b111;
		// Update mailbox repr first
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		mailbox[move.dst()] = mailbox[move.src()];
		mailbox[move.src()] = NO_PIECE;
		// Update piece and occupancy bitboard
		piece_boards[piece] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		// Handle castling rights
		if (piece == KING) {
			castling &= ~((WHITE_OO | WHITE_OOO) << (side << 1));
		} else if (piece == ROOK) {
			if (move.src() == SQ_A1)
				castling &= ~WHITE_OOO;
			else if (move.src() == SQ_H1)
				castling &= ~WHITE_OO;
			else if (move.src() == SQ_A8)
				castling &= ~BLACK_OOO;
			else if (move.src() == SQ_H8)
				castling &= ~BLACK_OO;
		} else {
			// Set EP square if applicable
			if (piece == PAWN && ((move.src() - move.dst()) & 0b1111) == 0)
				tmp_ep_square = Square((move.src() + move.dst()) >> 1);
			else
				tmp_ep_square = SQ_NONE;
		}
	}
	// Update EP square
	if (ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[ep_square & 0b111];
	if (tmp_ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[tmp_ep_square & 0b111];
	ep_square = tmp_ep_square;
	// Switch sides
	side = !side;
	zobrist ^= zobrist_side;
	// Update castling rights
	zobrist ^= zobrist_castling[castling] ^ zobrist_castling[move_hist.top().prev_castling()];

	halfmove++;

	hash_hist.push_back(zobrist);

#ifdef HASHCHECK
	old_hash = zobrist;
	recompute_hash();
	if (zobrist != old_hash) {
		print_board();
		std::cerr << "Hash mismatch after make: expected " << zobrist << " got " << old_hash << std::endl;
		abort();
	}
#endif

#ifdef SANCHECK
	char after[64];
	if (sanity_check(after)) {
		for (int i = 0; i < 64; i++) {
			std::cout << before[i];
			if (i % 8 == 7)
				std::cout << '\n';
			else
				std::cout << ' ';
		}
		std::cout << (!side ? "black " : "white ") << move.to_string() << std::endl;
		for (int i = 0; i < 64; i++) {
			std::cout << after[i];
			if (i % 8 == 7)
				std::cout << '\n';
			else
				std::cout << ' ';
		}
		abort();
	}
#endif
}

void Board::unmake_move() {
	// char before[64];
	// sanity_check(before);

#ifdef HASHCHECK
	uint64_t old_hash = zobrist;
	recompute_hash();
	if (old_hash != zobrist) {
		std::cerr << "Hash mismatch before unmake: expected " << zobrist << " got " << old_hash << '\n';
		abort();
	}
#endif

	hash_hist.pop_back();

	// Switch sides first
	side = !side;
	zobrist ^= zobrist_side;

	HistoryEntry prev = move_hist.top();
	move_hist.pop();
	Move move = prev.move();
	if (move.data == 0) {
		// Null move, do nothing on the board, but recover metadata
	} else if (move.type() == PROMOTION) {
		// Remove the piece on the dst and add the pawn on the src
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.dst()][prev.prev_piece()];
		mailbox[move.src()] = Piece(PAWN + ((!!side) << 3));
		mailbox[move.dst()] = prev.prev_piece();
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]];
		piece_boards[PAWN] ^= square_bits(move.src());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[((move.data >> 12) & 0b11) + KNIGHT] ^= square_bits(move.dst());
		// Handle captures
		if (prev.prev_piece() != NO_PIECE) { // If there was a capture
			// Add whatever piece it was
			uint8_t piece = prev.prev_piece() & 0b111;
			piece_boards[piece] ^= square_bits(move.dst());
			piece_boards[OPPOCC(side)] ^= square_bits(move.dst());
		}
	} else if (move.type() == EN_PASSANT) {
		// Remove the pawn on the dst and add the pawn on the src and the taken pawn
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.src()][mailbox[move.dst()]];
		mailbox[move.src()] = mailbox[move.dst()];
		mailbox[move.dst()] = NO_PIECE;
		mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)] = Piece(WHITE_PAWN + ((!side) << 3));
		zobrist ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]]; // Taken pawn
		piece_boards[PAWN] ^= square_bits(move.src()) | square_bits(move.dst()) | square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
	} else if (move.type() == CASTLING) {
		if (move.data == 0b1100000100000110) {
			// White O-O
			mailbox[SQ_G1] = NO_PIECE;
			mailbox[SQ_E1] = Piece(WHITE_KING);
			mailbox[SQ_F1] = NO_PIECE;
			mailbox[SQ_H1] = Piece(WHITE_ROOK);
			piece_boards[OCC(WHITE)] ^= square_bits(SQ_E1) | square_bits(SQ_G1) | square_bits(SQ_H1) | square_bits(SQ_F1);
			piece_boards[KING] ^= square_bits(SQ_E1) | square_bits(SQ_G1);
			piece_boards[ROOK] ^= square_bits(SQ_H1) | square_bits(SQ_F1);
			zobrist ^= zobrist_square[SQ_E1][Piece(WHITE_KING)] ^ zobrist_square[SQ_G1][Piece(WHITE_KING)];
			zobrist ^= zobrist_square[SQ_H1][Piece(WHITE_ROOK)] ^ zobrist_square[SQ_F1][Piece(WHITE_ROOK)];
		} else if (move.data == 0b1100000100000010) {
			// White O-O-O
			mailbox[SQ_C1] = NO_PIECE;
			mailbox[SQ_E1] = Piece(WHITE_KING);
			mailbox[SQ_D1] = NO_PIECE;
			mailbox[SQ_A1] = Piece(WHITE_ROOK);
			piece_boards[OCC(WHITE)] ^= square_bits(SQ_E1) | square_bits(SQ_C1) | square_bits(SQ_A1) | square_bits(SQ_D1);
			piece_boards[KING] ^= square_bits(SQ_E1) | square_bits(SQ_C1);
			piece_boards[ROOK] ^= square_bits(SQ_A1) | square_bits(SQ_D1);
			zobrist ^= zobrist_square[SQ_E1][Piece(WHITE_KING)] ^ zobrist_square[SQ_C1][Piece(WHITE_KING)];
			zobrist ^= zobrist_square[SQ_A1][Piece(WHITE_ROOK)] ^ zobrist_square[SQ_D1][Piece(WHITE_ROOK)];
		} else if (move.data == 0b1100111100111110) {
			// Black O-O
			mailbox[SQ_G8] = NO_PIECE;
			mailbox[SQ_E8] = Piece(BLACK_KING);
			mailbox[SQ_F8] = NO_PIECE;
			mailbox[SQ_H8] = Piece(BLACK_ROOK);
			piece_boards[OCC(BLACK)] ^= square_bits(SQ_E8) | square_bits(SQ_G8) | square_bits(SQ_H8) | square_bits(SQ_F8);
			piece_boards[KING] ^= square_bits(SQ_E8) | square_bits(SQ_G8);
			piece_boards[ROOK] ^= square_bits(SQ_H8) | square_bits(SQ_F8);
			zobrist ^= zobrist_square[SQ_E8][Piece(BLACK_KING)] ^ zobrist_square[SQ_G8][Piece(BLACK_KING)];
			zobrist ^= zobrist_square[SQ_H8][Piece(BLACK_ROOK)] ^ zobrist_square[SQ_F8][Piece(BLACK_ROOK)];
		} else if (move.data == 0b1100111100111010) {
			// Black O-O-O
			mailbox[SQ_C8] = NO_PIECE;
			mailbox[SQ_E8] = Piece(BLACK_KING);
			mailbox[SQ_D8] = NO_PIECE;
			mailbox[SQ_A8] = Piece(BLACK_ROOK);
			piece_boards[OCC(BLACK)] ^= square_bits(SQ_E8) | square_bits(SQ_C8) | square_bits(SQ_A8) | square_bits(SQ_D8);
			piece_boards[KING] ^= square_bits(SQ_E8) | square_bits(SQ_C8);
			piece_boards[ROOK] ^= square_bits(SQ_A8) | square_bits(SQ_D8);
			zobrist ^= zobrist_square[SQ_E8][Piece(BLACK_KING)] ^ zobrist_square[SQ_C8][Piece(BLACK_KING)];
			zobrist ^= zobrist_square[SQ_A8][Piece(BLACK_ROOK)] ^ zobrist_square[SQ_D8][Piece(BLACK_ROOK)];
		} else {
			std::cerr << "Il faut que tu meures" << std::endl;
			volatile int *p = 0;
			*p = move.type();
		}
	} else {
		// Get piece that is moving
		uint8_t piece = mailbox[move.dst()] & 0b111;
		// Update mailbox repr first
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.src()][mailbox[move.dst()]];
		zobrist ^= zobrist_square[move.dst()][prev.prev_piece()];
		mailbox[move.src()] = mailbox[move.dst()];
		mailbox[move.dst()] = prev.prev_piece();
		// Update piece and occupancy bitboard
		piece_boards[piece] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		// Handle captures
		if (prev.prev_piece() != NO_PIECE) { // If there was a capture
			// Add whatever piece it was
			piece = prev.prev_piece() & 0b111;
			piece_boards[piece] ^= square_bits(move.dst());
			piece_boards[OPPOCC(side)] ^= square_bits(move.dst());
		}
	}

	// Update EP square
	if (ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[ep_square & 0b111];
	if (prev.prev_ep() != SQ_NONE)
		zobrist ^= zobrist_ep[prev.prev_ep() & 0b111];
	ep_square = prev.prev_ep();
	// Update castling rights
	int old_castling = castling;
	zobrist ^= zobrist_castling[castling] ^ zobrist_castling[prev.prev_castling()];
	castling = prev.prev_castling();

	halfmove = halfmove_hist.top();
	halfmove_hist.pop();

#ifdef HASHCHECK
	old_hash = zobrist;
	recompute_hash();
	if (zobrist != old_hash) {
		print_board();
		std::cerr << "Hash mismatch after unmake: expected " << zobrist << " got " << old_hash << '\n';
		std::cerr << prev.move().to_string() << std::endl;
		abort();
	}
#endif

	// char after[64];
	// if (sanity_check(after)) {
	// 	for (int i = 0; i < 64; i++) {
	// 		std::cout << before[i];
	// 		if (i % 8 == 7)
	// 			std::cout << '\n';
	// 		else
	// 			std::cout << ' ';
	// 	}
	// 	std::cout << "Unmaking: " << move.to_string() << std::endl;
	// 	for (int i = 0; i < 64; i++) {
	// 		std::cout << after[i];
	// 		if (i % 8 == 7)
	// 			std::cout << '\n';
	// 		else
	// 			std::cout << ' ';
	// 	}
	// 	abort();
	// }
}

void Board::recompute_hash() {
	zobrist = 0;
	for (int i = 0; i < 64; i++) {
		zobrist ^= zobrist_square[i][mailbox[i]];
	}
	zobrist ^= zobrist_castling[castling];
	if (ep_square != SQ_NONE) {
		zobrist ^= zobrist_ep[ep_square & 0b111];
	}
	zobrist ^= zobrist_side * side;
}

bool Board::threefold() {
	int cnt = 0;
	for (const uint64_t h : hash_hist) {
		if (h == zobrist)
			cnt++;
		if (cnt >= 3)
			return true;
	}
	return false;
}
