#include "bitboard.hpp"
#include <cctype>
#include <random>

extern bool dfrc_uci;

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
	if (!dfrc_uci && type() == CASTLING) {
		if (dst() > src())
			return str + 'g' + (char)('1' + (dst() >> 3));
		else
			return str + 'c' + (char)('1' + (dst() >> 3));
	} else {
		str += (char)('a' + (dst() & 0b111));
		str += (char)('1' + (dst() >> 3));
	}
	if ((data & 0xc000) == PROMOTION) {
		str += piecetype_letter[((data >> 12) & 0b11) + KNIGHT];
	}
	return str;
}

Move Move::from_string(const std::string &str, const void *b) {
	const Board *board = (const Board *)b;

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
			if (dfrc_uci) {
				if ((board->mailbox[src] & 7) == KING && board->mailbox[dst] == (ROOK | (board->mailbox[src] & 8)))
					return Move::make<CASTLING>(src, dst);
				else
					return Move(src, dst);
			} else {
				if (str == "e1g1")
					return Move::make<CASTLING>(src, board->rook_pos[1]);
				if (str == "e1c1")
					return Move::make<CASTLING>(src, board->rook_pos[0]);
				if (str == "e8g8")
					return Move::make<CASTLING>(src, board->rook_pos[3]);
				if (str == "e8c8")
					return Move::make<CASTLING>(src, board->rook_pos[2]);
			}
		} else if ((board->mailbox[src] & 7) == PAWN && dst == ((const Board *)b)->ep_square) {
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
	Bitboard rooks = piece_boards[ROOK];
	if (fen[inputIdx] != '-') {
		char king_file = (_tzcnt_u64(piece_boards[KING] & piece_boards[OCC(WHITE)])) + 'A';
		if (fen[inputIdx] == 'K') {
			castling |= WHITE_OO;
			inputIdx++;
			Square king_square = make_square(File(king_file - 'A'), RANK_1);
			Bitboard mask = ~(square_bits(king_square) - 1);
			rook_pos[0] = Square(_tzcnt_u64(rooks & mask & piece_boards[OCC(WHITE)]));
		} else if (fen[inputIdx] <= 'H' && fen[inputIdx] > king_file) {
			dfrc_uci = true;
			castling |= WHITE_OO;
			inputIdx++;
			rook_pos[0] = make_square(File(fen[inputIdx - 1] - 'A'), RANK_1);
		}

		if (fen[inputIdx] == 'Q') {
			castling |= WHITE_OOO;
			inputIdx++;
			Square king_square = make_square(File(king_file - 'A'), RANK_1);
			Bitboard mask = (square_bits(king_square) - 1);
			rook_pos[1] = Square(_tzcnt_u64(rooks & mask & piece_boards[OCC(WHITE)]));
		} else if (fen[inputIdx] >= 'A' && fen[inputIdx] < king_file) {
			dfrc_uci = true;
			castling |= WHITE_OOO;
			inputIdx++;
			rook_pos[1] = make_square(File(fen[inputIdx - 1] - 'A'), RANK_1);
		}

		king_file = (_tzcnt_u64(piece_boards[KING] & piece_boards[OCC(BLACK)]) & 7) + 'a';
		if (fen[inputIdx] == 'k') {
			castling |= BLACK_OO;
			inputIdx++;
			Square king_square = make_square(File(king_file - 'a'), RANK_8);
			Bitboard mask = ~(square_bits(king_square) - 1);
			rook_pos[2] = Square(_tzcnt_u64(rooks & mask & piece_boards[OCC(BLACK)]));
		} else if (fen[inputIdx] <= 'h' && fen[inputIdx] > king_file) {
			dfrc_uci = true;
			castling |= BLACK_OO;
			inputIdx++;
			rook_pos[2] = make_square(File(fen[inputIdx - 1] - 'a'), RANK_8);
		}

		if (fen[inputIdx] == 'q') {
			castling |= BLACK_OOO;
			inputIdx++;
			Square king_square = make_square(File(king_file - 'a'), RANK_8);
			Bitboard mask = (square_bits(king_square) - 1);
			rook_pos[3] = Square(63 - _lzcnt_u64(rooks & mask & piece_boards[OCC(BLACK)]));
		} else if (fen[inputIdx] >= 'a' && fen[inputIdx] < king_file) {
			dfrc_uci = true;
			castling |= BLACK_OOO;
			inputIdx++;
			rook_pos[3] = make_square(File(fen[inputIdx - 1] - 'a'), RANK_8);
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

void Board::reset_board() {
	piece_boards[0] = Rank2Bits | Rank7Bits;  // Pawns
	piece_boards[1] = square_bits(SQ_B1) | square_bits(SQ_G1) | square_bits(SQ_B8) | square_bits(SQ_G8);  // Knights
	piece_boards[2] = square_bits(SQ_C1) | square_bits(SQ_F1) | square_bits(SQ_C8) | square_bits(SQ_F8);  // Bishops
	piece_boards[3] = square_bits(SQ_A1) | square_bits(SQ_H1) | square_bits(SQ_A8) | square_bits(SQ_H8);  // Rooks
	piece_boards[4] = square_bits(SQ_D1) | square_bits(SQ_D8);  // Queens
	piece_boards[5] = square_bits(SQ_E1) | square_bits(SQ_E8);  // Kings
	piece_boards[6] = Rank1Bits | Rank2Bits;  // White occupancy
	piece_boards[7] = Rank7Bits | Rank8Bits;  // Black occupancy

	Piece starting_mailbox[64] = {
		WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK,
		WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,  WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,
		NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,    NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,
		NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,    NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,
		NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,    NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,
		NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,    NO_PIECE,   NO_PIECE,     NO_PIECE,     NO_PIECE,
		BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,  BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,
		BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK
	};
	memcpy(mailbox, starting_mailbox, sizeof(mailbox));

	side = WHITE;
	halfmove = 0;
	castling = 0xf;  // 1111
	ep_square = SQ_NONE;
	rook_pos[0] = SQ_H1;
	rook_pos[1] = SQ_A1;
	rook_pos[2] = SQ_H8;
	rook_pos[3] = SQ_A8;

	while (!move_hist.empty()) move_hist.pop();
	while (!halfmove_hist.empty()) halfmove_hist.pop();

	hash_hist.clear();

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
		res += '-';
	} else {
		if (castling & WHITE_OO)
			res += dfrc_uci ? (rook_pos[0] + 'A') : 'K';
		if (castling & WHITE_OOO)
			res += dfrc_uci ? (rook_pos[1] + 'A') : 'Q';
		if (castling & BLACK_OO)
			res += dfrc_uci ? (rook_pos[2] & 7) + 'a' : 'k';
		if (castling & BLACK_OOO)
			res += dfrc_uci ? (rook_pos[3] & 7) + 'a' : 'q';
	}
	
	// En passant square
	if (ep_square >= SQ_NONE) {
		res += " - ";
	} else {
		res += ' ';
		res += 'a' + (ep_square & 0b111);
		res += '1' + (ep_square >> 3);
		res += ' ';
	}
	// Halfmove clock
	res += std::to_string(halfmove);

	// Fullmove number
	res += ' ';
	res += std::to_string((hash_hist.size() / 2) + 1);
	return res;
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
				std::cerr << "Occupancy and mailbox differ on square " << char(file + 'a') << char(rank + '1') << '\n';
				error = 1;
				continue;
			}
			if (sanity & bits) { // Occupancy and collective piece boards differ
				if (occ & bits) { // Occupied but no piece specified
					print[printIdx] = '?';
					std::cerr << "Occupied but no piece specified on square " << char(file + 'a') << char(rank + '1') << '\n';
					error = 1;
				} else { // Piece specified but no occupancy
					print[printIdx] = '!';
					std::cerr << "Piece specified but no occupancy on square " << char(file + 'a') << char(rank + '1') << '\n';
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
					std::cerr << "More than two pieces on square " << char(file + 'a') << char(rank + '1') << '\n';
					error = 1;
					break;
				}
				put = true;
				if (mailbox[rank * 8 + file] != type + (!!(piece_boards[OCC(BLACK)] & bits) << 3)) { // Bitboard and mailbox representations differ
					print[printIdx] = '$';
					std::cerr << "Bitboard and mailbox representations differ on square " << char(file + 'a') << char(rank + '1') << '\n';
					error = 1;
					break;
				}
				print[printIdx] = piecetype_letter[type & 7] - (!(piece_boards[OCC(BLACK)] & bits) << 5);
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

	if (ep_square >= SQ_NONE)
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

void Board::print_board_pretty(bool print_meta) const {
	const std::string pieces[] = {
		// "♟", "♞", "♝", "♜", "♛", "♚", "", "",
		// "♙", "♘", "♗", "♖", "♕", "♔", "", "",
		"P", "N", "B", "R", "Q", "K", "", "",
		"p", "n", "b", "r", "q", "k", "", "", // some terminals don't support chess pieces
	};

	std::cout << "\n" BORDER "  ┌──────────────────────────┐" RESET << std::endl;

	for (int rank = RANK_8; rank >= RANK_1; rank--) {
		std::cout << BORDER "  │ " RESET;

		for (int file = FILE_A; file <= FILE_H; file++) {
			Square sq = make_square((File)file, (Rank)rank);
			Piece piece = mailbox[sq];

			bool is_light = ((rank + file) % 2) == 0;
			std::string bg_color = is_light ? LIGHT_SQUARE : DARK_SQUARE;

			std::cout << bg_color;
			if (piece == NO_PIECE) {
				std::cout << "   "; // 3 spaces
			} else {
				std::cout << " " << pieces[piece] << " ";
			}
			std::cout << RESET;
		}
		std::cout << BORDER " │ " COORDS << (rank + 1) << RESET << std::endl;
	}

	std::cout << BORDER "  └──────────────────────────┘" RESET << std::endl;
	std::cout << BORDER "    " RESET;
	for (char file = 'a'; file <= 'h'; file++) {
		std::cout << COORDS " " << file << " " RESET;
	}
	std::cout << std::endl;

	if (print_meta) {
		std::cout << "\n" COORDS "FEN: " RESET << get_fen() << std::endl;
		std::cout << COORDS "Side to move: " RESET << (side ? "Black" : "White") << std::endl;

		std::cout << COORDS "Castling: " RESET;
		if (castling == NO_CASTLE) {
			std::cout << "-";
		} else {
			if (castling & WHITE_OO) std::cout << "K";
			if (castling & WHITE_OOO) std::cout << "Q";
			if (castling & BLACK_OO) std::cout << "k";
			if (castling & BLACK_OOO) std::cout << "q";
		}
		std::cout << std::endl;

		std::cout << COORDS "En passant: " << RESET;
		if (ep_square >= SQ_NONE) {
			std::cout << "-";
		} else {
			std::cout << to_string(ep_square);
		}
		std::cout << std::endl;

		std::cout << COORDS "Halfmove clock: " RESET << (int)halfmove << std::endl;
		std::cout << std::endl;
	}
}

void Board::make_move(Move move) {
#ifdef SANCHECK
	char before[64];
	sanity_check(before);
#endif

#ifdef HASHCHECK
	uint64_t old_hash = zobrist;
	uint64_t old_piece_hashes[15];
	memcpy(old_piece_hashes, piece_hashes, sizeof(piece_hashes));
	recompute_hash();
	if (old_hash != zobrist) {
		std::cerr << "Hash mismatch before move: expected " << zobrist << " got " << old_hash << '\n';
		abort();
	}
	for (int i = 0; i < 15; i++) {
		if (old_piece_hashes[i] != piece_hashes[i]) {
			std::cerr << "Piece hash mismatch before move for piece " << i << ": expected " << piece_hashes[i] << " got " << old_piece_hashes[i] << '\n';
			abort();
		}
	}
#endif

	// Add move to move history
	move_hist.push(HistoryEntry(move, mailbox[move.dst()], castling, ep_square));
	halfmove_hist.push(halfmove);
	Square tmp_ep_square = SQ_NONE;

	// Handle captures
	if (move.data != 0 && is_capture(move)) {
		// Remove whatever piece it was
		uint8_t piece = mailbox[move.dst()] & 0b111;
		piece_boards[piece] ^= square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(move.dst());
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]];
		piece_hashes[mailbox[move.dst()]] ^= zobrist_square[move.dst()][mailbox[move.dst()]];

		if (piece == ROOK) {
			if (move.dst() == rook_pos[1])
				castling &= ~WHITE_OOO;
			else if (move.dst() == rook_pos[0])
				castling &= ~WHITE_OO;
			else if (move.dst() == rook_pos[3])
				castling &= ~BLACK_OOO;
			else if (move.dst() == rook_pos[2])
				castling &= ~BLACK_OO;
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
		piece_hashes[mailbox[move.src()]] ^= zobrist_square[move.src()][mailbox[move.src()]];
		mailbox[move.src()] = NO_PIECE;
		mailbox[move.dst()] = Piece(move.promotion() + ((!!side) << 3) + KNIGHT);
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]];
		piece_hashes[mailbox[move.dst()]] ^= zobrist_square[move.dst()][mailbox[move.dst()]];
		piece_boards[PAWN] ^= square_bits(move.src());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[move.promotion() + KNIGHT] ^= square_bits(move.dst());
	} else if (move.type() == EN_PASSANT) {
		// Remove the pawn on the src and the taken pawn, then add the pawn on the dst
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		zobrist ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]]; // Taken pawn
		piece_hashes[mailbox[move.src()]] ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		piece_hashes[mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]] ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]];
		mailbox[move.dst()] = mailbox[move.src()];
		mailbox[move.src()] = NO_PIECE;
		mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)] = NO_PIECE;
		piece_boards[PAWN] ^= square_bits(move.src()) | square_bits(move.dst()) | square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
	} else if (move.type() == CASTLING) {
		// Remove castling rights
		castling &= ~((WHITE_OO | WHITE_OOO) << (side + side));

		Square king_from = move.src();
		Square king_to = move.dst() > move.src() ? make_square(FILE_G, Rank(king_from >> 3)) : make_square(FILE_C, Rank(king_from >> 3));

		Square rook_from = move.dst();
		Square rook_to = move.dst() > move.src() ? make_square(FILE_F, Rank(rook_from >> 3)) : make_square(FILE_D, Rank(rook_from >> 3));

		Piece king_piece = Piece(KING + (side << 3));
		Piece rook_piece = Piece(ROOK + (side << 3));

		zobrist ^= zobrist_square[king_from][king_piece] ^ zobrist_square[king_to][king_piece];
		zobrist ^= zobrist_square[rook_from][rook_piece] ^ zobrist_square[rook_to][rook_piece];
		piece_hashes[king_piece] ^= zobrist_square[king_from][king_piece] ^ zobrist_square[king_to][king_piece];
		piece_hashes[rook_piece] ^= zobrist_square[rook_from][rook_piece] ^ zobrist_square[rook_to][rook_piece];
		mailbox[king_from] = NO_PIECE;
		mailbox[rook_from] = NO_PIECE;
		mailbox[king_to] = king_piece;
		mailbox[rook_to] = rook_piece;
		piece_boards[KING] ^= square_bits(king_from) ^ square_bits(king_to);
		piece_boards[ROOK] ^= square_bits(rook_from) ^ square_bits(rook_to);
		piece_boards[OCC(side)] ^= (square_bits(king_from) ^ square_bits(king_to)) ^ (square_bits(rook_from) ^ square_bits(rook_to));
	} else {
		// Get piece that is moving
		uint8_t piece = mailbox[move.src()] & 0b111;
		// Update mailbox repr first
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		piece_hashes[mailbox[move.src()]] ^= zobrist_square[move.src()][mailbox[move.src()]] ^ zobrist_square[move.dst()][mailbox[move.src()]];
		mailbox[move.dst()] = mailbox[move.src()];
		mailbox[move.src()] = NO_PIECE;
		// Update piece and occupancy bitboard
		piece_boards[piece] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		// Handle castling rights
		if (piece == KING) {
			castling &= ~((WHITE_OO | WHITE_OOO) << (side << 1));
		} else if (piece == ROOK) {
			if (move.src() == rook_pos[1])
				castling &= ~WHITE_OOO;
			else if (move.src() == rook_pos[0])
				castling &= ~WHITE_OO;
			else if (move.src() == rook_pos[3])
				castling &= ~BLACK_OOO;
			else if (move.src() == rook_pos[2])
				castling &= ~BLACK_OO;
		} else {
			// Set EP square if applicable
			if (piece == PAWN && ((move.src() - move.dst()) & 0b1111) == 0)
				tmp_ep_square = Square((move.src() + move.dst()) >> 1);
			else
				tmp_ep_square = SQ_NONE;
		}
	}

	// Switch sides
	side = !side;
	zobrist ^= zobrist_side;
	// Update castling rights
	zobrist ^= zobrist_castling[castling] ^ zobrist_castling[move_hist.top().prev_castling()];

	// Remove EP square
	if (ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[ep_square & 0b111];

	hash_hist.push_back(zobrist);

	// Set new EP square
	if (tmp_ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[tmp_ep_square & 0b111];
	ep_square = tmp_ep_square;

	halfmove++;

#ifdef HASHCHECK
	old_hash = zobrist;
	memcpy(old_piece_hashes, piece_hashes, sizeof(piece_hashes));
	recompute_hash();
	if (zobrist != old_hash) {
		print_board();
		std::cerr << "Hash mismatch after make: expected " << old_hash << " got " << zobrist << std::endl;
		std::cerr << "Move: " << move.to_string() << std::endl;
		abort();
	}
	for (int i = 0; i < 15; i++) {
		if (old_piece_hashes[i] != piece_hashes[i]) {
			print_board();
			std::cerr << "Piece hash mismatch after make for piece " << i << ": expected " << old_piece_hashes[i] << " got " << piece_hashes[i] << std::endl;
			std::cerr << "Move: " << move.to_string() << std::endl;
			abort();
		}
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
		std::cout << "Sanity check failed after make " << move.to_string() << std::endl;
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
#ifdef SANCHECK
	char before[64];
	sanity_check(before);
#endif

#ifdef HASHCHECK
	uint64_t old_hash = zobrist;
	uint64_t old_piece_hashes[15];
	memcpy(old_piece_hashes, piece_hashes, sizeof(piece_hashes));
	recompute_hash();
	if (old_hash != zobrist) {
		std::cerr << "Hash mismatch before unmake: expected " << zobrist << " got " << old_hash << '\n';
		abort();
	}
	for (int i = 0; i < 15; i++) {
		if (old_piece_hashes[i] != piece_hashes[i]) {
			std::cerr << "Piece hash mismatch before unmake for piece " << i << ": expected " << piece_hashes[i] << " got " << old_piece_hashes[i] << '\n';
			abort();
		}
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
		piece_hashes[mailbox[move.dst()]] ^= zobrist_square[move.dst()][mailbox[move.dst()]];
		piece_hashes[prev.prev_piece()] ^= zobrist_square[move.dst()][prev.prev_piece()];
		mailbox[move.src()] = Piece(PAWN + ((!!side) << 3));
		mailbox[move.dst()] = prev.prev_piece();
		zobrist ^= zobrist_square[move.src()][mailbox[move.src()]];
		piece_hashes[mailbox[move.src()]] ^= zobrist_square[move.src()][mailbox[move.src()]];
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
		piece_hashes[mailbox[move.dst()]] ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.src()][mailbox[move.dst()]];
		mailbox[move.src()] = mailbox[move.dst()];
		mailbox[move.dst()] = NO_PIECE;
		mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)] = Piece(WHITE_PAWN + ((!side) << 3));
		zobrist ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]]; // Taken pawn
		piece_hashes[mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]] ^= zobrist_square[(move.src() & 0b111000) | (move.dst() & 0b111)][mailbox[(move.src() & 0b111000) | (move.dst() & 0b111)]];
		piece_boards[PAWN] ^= square_bits(move.src()) | square_bits(move.dst()) | square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
		piece_boards[OCC(side)] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[OPPOCC(side)] ^= square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
	} else if (move.type() == CASTLING) {
		Square king_from = move.src();
		Square king_to = move.dst() > move.src() ? make_square(FILE_G, Rank(king_from >> 3)) : make_square(FILE_C, Rank(king_from >> 3));

		Square rook_from = move.dst();
		Square rook_to = move.dst() > move.src() ? make_square(FILE_F, Rank(rook_from >> 3)) : make_square(FILE_D, Rank(rook_from >> 3));

		Piece king_piece = Piece(KING + (side << 3));
		Piece rook_piece = Piece(ROOK + (side << 3));

		zobrist ^= zobrist_square[king_from][king_piece] ^ zobrist_square[king_to][king_piece];
		zobrist ^= zobrist_square[rook_from][rook_piece] ^ zobrist_square[rook_to][rook_piece];
		piece_hashes[king_piece] ^= zobrist_square[king_from][king_piece] ^ zobrist_square[king_to][king_piece];
		piece_hashes[rook_piece] ^= zobrist_square[rook_from][rook_piece] ^ zobrist_square[rook_to][rook_piece];
		mailbox[king_to] = NO_PIECE;
		mailbox[rook_to] = NO_PIECE;
		mailbox[king_from] = king_piece;
		mailbox[rook_from] = rook_piece;
		piece_boards[KING] ^= square_bits(king_from) ^ square_bits(king_to);
		piece_boards[ROOK] ^= square_bits(rook_from) ^ square_bits(rook_to);
		piece_boards[OCC(side)] ^= (square_bits(king_from) ^ square_bits(king_to)) ^ (square_bits(rook_from) ^ square_bits(rook_to));
	} else {
		// Get piece that is moving
		uint8_t piece = mailbox[move.dst()] & 0b111;
		// Update mailbox repr first
		zobrist ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.src()][mailbox[move.dst()]];
		zobrist ^= zobrist_square[move.dst()][prev.prev_piece()];
		piece_hashes[mailbox[move.dst()]] ^= zobrist_square[move.dst()][mailbox[move.dst()]] ^ zobrist_square[move.src()][mailbox[move.dst()]];
		piece_hashes[prev.prev_piece()] ^= zobrist_square[move.dst()][prev.prev_piece()];
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
	memcpy(old_piece_hashes, piece_hashes, sizeof(piece_hashes));
	recompute_hash();
	if (zobrist != old_hash) {
		print_board();
		std::cerr << "Hash mismatch after unmake: expected " << old_hash << " got " << zobrist << std::endl;
		std::cerr << "Move: " << move.to_string() << std::endl;
		abort();
	}
	for (int i = 0; i < 15; i++) {
		if (old_piece_hashes[i] != piece_hashes[i]) {
			print_board();
			std::cerr << "Piece hash mismatch after unmake for piece " << i << ": expected " << old_piece_hashes[i] << " got " << piece_hashes[i] << std::endl;
			std::cerr << "Move: " << move.to_string() << std::endl;
			abort();
		}
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
		std::cout << "Sanity check failed after unmake " << move.to_string() << " from " << (!side ? "black " : "white ") << std::endl;
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

void Board::recompute_hash() {
	zobrist = 0;
	for (int i = 0; i < 15; i++) piece_hashes[i] = 0;
	for (int i = 0; i < 64; i++) {
		PieceType pt = PieceType(mailbox[i] & 7);
		zobrist ^= zobrist_square[i][mailbox[i]];
		piece_hashes[mailbox[i]] ^= zobrist_square[i][mailbox[i]];
	}
	zobrist ^= zobrist_castling[castling];
	if (ep_square != SQ_NONE) {
		zobrist ^= zobrist_ep[ep_square & 0b111];
	}
	zobrist ^= zobrist_side * side;
}

bool Board::threefold(int ply) {
	int cnt = 0, plies = 0;
	for (int idx = hash_hist.size() - 1; idx >= 0; idx--) {
		const uint64_t& h = hash_hist[idx];
		if (h == zobrist)
			cnt++;
		if (plies < ply && cnt >= 2)
			return true;
		plies++;
		if (cnt >= 3) return true;
	}
	return false;
}

bool Board::insufficient_material() const {
	Bitboard all_pieces = piece_boards[PAWN] | piece_boards[ROOK] | piece_boards[QUEEN];
	if (all_pieces != 0) return false; // There is at least one pawn, rook or queen

	int nknights = _mm_popcnt_u64(piece_boards[KNIGHT]);
	int nbishops = _mm_popcnt_u64(piece_boards[BISHOP]);

	if (nknights > 2) return false; // More than 2 knights

	if (nbishops > 2) return false; // More than 2 bishops

	if (nbishops == 2) {
		// Check if they are on same color
		Bitboard bishops = piece_boards[BISHOP];
		Square first_bishop = (Square)_tzcnt_u64(bishops);
		bishops ^= square_bits(first_bishop);
		Square second_bishop = (Square)_tzcnt_u64(bishops);
		if (((first_bishop ^ second_bishop) & 0b1) == 0) {
			return true; // Both bishops on same color
		} else {
			return false;
		}
	}

	if (nbishops == 1) {
		// Check if there is any knight
		if (nknights >= 1) return false;
		return true;
	}

	// No bishops
	if (nknights <= 1) return true;

	return false;
}

uint64_t Board::pawn_hash() const {
	return piece_hashes[WHITE_PAWN] ^ piece_hashes[BLACK_PAWN];
}

uint64_t Board::nonpawn_hash(bool color) const {
	return piece_hashes[KING + (color << 3)] ^ piece_hashes[QUEEN + (color << 3)] ^ piece_hashes[ROOK + (color << 3)] ^ piece_hashes[BISHOP + (color << 3)] ^ piece_hashes[KNIGHT + (color << 3)];
}

uint64_t Board::major_hash() const {
	return piece_hashes[WHITE_KING] ^ piece_hashes[WHITE_QUEEN] ^ piece_hashes[WHITE_ROOK] ^ piece_hashes[BLACK_KING] ^ piece_hashes[BLACK_QUEEN] ^ piece_hashes[BLACK_ROOK];
}

uint64_t Board::minor_hash() const {
	return piece_hashes[WHITE_KING] ^ piece_hashes[WHITE_BISHOP] ^ piece_hashes[WHITE_KNIGHT] ^ piece_hashes[BLACK_KING] ^ piece_hashes[BLACK_BISHOP] ^ piece_hashes[BLACK_KNIGHT];
}
