#include "bitboard.hpp"
// #include "moves.hpp"

void Board::load_fen(std::string fen) {
	memset(piece_boards, 0, sizeof(piece_boards));
	memset(control, 0, sizeof(control));
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
		ep_square = Square((fen[inputIdx] - 'a') * 8 + (fen[inputIdx + 1] - '1'));
		inputIdx += 3;
	} else {
		inputIdx += 2;
	}

	// Ignore the rest (who cares anyways)
}

void Board::print_board() {
#ifndef NODEBUG
	char print[64];
	// Start at -1 because we increment before processing to guarantee it happens every time
	int printIdx = -1;
	// Occupancy
	uint64_t occ = piece_boards[6] | piece_boards[7];
	// Used for sanity checks during debugging (piece set but no occupancy)
	uint64_t sanity = (piece_boards[0] | piece_boards[1] | piece_boards[2] | piece_boards[3] | piece_boards[4] | piece_boards[5]) ^ occ;
	for (uint16_t rank = RANK_8; rank <= RANK_8; rank--) { // Catches wrap-around subtraction
		for (uint16_t file = FILE_A; file <= FILE_H; file++) {
			uint64_t bits = square_bits((Rank)rank, (File)file);
			printIdx++;
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
				if (mailbox[rank * 8 + file] != type + (!!(piece_boards[7] & bits) << 3)) { // Bitboard and mailbox representations differ
					print[printIdx] = '$';
					break;
				}
				print[printIdx] = piecetype_letter[type] + (!!(piece_boards[7] & bits) << 5);
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
	for (uint16_t rank = RANK_8; rank <= RANK_8; rank--) { // Catches wrap-around subtraction
		for (uint16_t file = FILE_A; file <= FILE_H; file++) {
			std::cout << piece_letter[mailbox[rank * 8 + file]];
			if (file == FILE_H)
				std::cout << '\n';
			else
				std::cout << ' ';
		}
	}
#endif
}

bool Board::make_move(Move move) {
	if (move.data == 0) {
		// Null move, do nothing, just change sides
	} else if (move.data & PROMOTION) {
		// Remove the pawn on the src and add the piece on the dst
		mailbox[move.src()] = NO_PIECE;
		mailbox[move.dst()] = Piece((move.data >> 12) & 0b11 + (!!side) << 3);
		piece_boards[PAWN] ^= square_bits(move.src());
		piece_boards[6 ^ side] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[(move.data >> 12) & 0b11] ^= square_bits(move.dst());
		// Handle captures
		if (piece_boards[7 ^ side] & square_bits(move.dst())) { // If opposite occupancy bit set on destination (capture)
			// Remove whatever piece it was
			uint8_t piece = mailbox[move.dst()] & 0b111;
			piece_boards[piece] ^= square_bits(move.dst());
			piece_boards[7 ^ side] ^= square_bits(move.dst());
		}
	} else if (move.data & EN_PASSANT) {
		// Remove the pawn on the src
		mailbox[move.src()] = NO_PIECE;
		piece_boards[PAWN] ^= square_bits(move.src()) | square_bits(move.dst()) | square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));
		piece_boards[6 ^ side] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[7 ^ side] ^= square_bits(Rank(move.src() >> 3), File(move.dst() & 0b111));

	} else if (move.data & CASTLING) {
		// Calculate where the rook is
		uint64_t rook_mask;
		if (move.dst() & 0b111 == FILE_C) {
			rook_mask = square_bits(Rank(move.src() >> 3), FILE_A);
			rook_mask |= rook_mask << 2;
		} else {
			rook_mask = square_bits(Rank(move.src() >> 3), FILE_H);
			rook_mask |= rook_mask >> 2;
		}
		piece_boards[6 ^ side] ^= square_bits(move.src()) | square_bits(move.dst()) | rook_mask;
		piece_boards[KING] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[ROOK] ^= rook_mask;
	} else {
		// Get piece that is moving
		uint8_t piece = mailbox[move.src()] & 0b111;
		// Update mailbox repr first
		mailbox[move.dst()] = mailbox[move.src()];
		mailbox[move.src()] = NO_PIECE;
		// Update piece and occupancy bitboard (same trick with the xor with 6)
		piece_boards[piece] ^= square_bits(move.src()) | square_bits(move.dst());
		piece_boards[6 ^ side] ^= square_bits(move.src()) | square_bits(move.dst());
		// Handle captures
		if (piece_boards[7 ^ side] & square_bits(move.dst())) { // If opposite occupancy bit set on destination (capture)
			// Remove whatever piece it was
			piece = mailbox[move.dst()] & 0b111;
			piece_boards[piece] ^= square_bits(move.dst());
			piece_boards[7 ^ side] ^= square_bits(move.dst());
		}
	}
	side = !side;
}

void Board::unmake_move() {
	/// TODO: IMPLEMENT
}

void Board::legal_moves(std::vector<Move> &moves) const {
	/// TODO: IMPLEMENT
}

uint64_t Board::hash() const {
	uint64_t hash = 0;
	// for (int i = 0; i < 6; i++) {
	// 	hash ^= pieces[i];
	// }
	// hash ^= castling;
	// hash ^= ep_square;
	// hash ^= control[0];
	// hash ^= control[1];
	// hash ^= side << 62;
	return hash;
}