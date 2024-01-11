#include "bitboard.hpp"
// #include "moves.hpp"

void Board::load_fen(std::string fen) {
	memset(pieces, 0, sizeof(pieces));
	memset(control, 0, sizeof(control));
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
			pieces[piece] |= square_bits((Rank)rank, (File)file);
			// Uppercase in ASCII is 0b010xxxxx, lowercase is 0b011xxxxx
			// Chooses either [6] or [7] (white or black occupancy)
			pieces[(6 ^ 0b10) ^ (cur >> 5)] |= square_bits((Rank)rank, (File)file);
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
		ep_square = (Square)((fen[inputIdx] - 'a') * 8 + (fen[inputIdx + 1] - '1'));
		inputIdx += 3;
	} else {
		inputIdx += 2;
	}

	// Ignore the rest (who cares anyways)
}

void Board::print_board() {
	char print[64];
	// Start at -1 because we increment before processing to guarantee it happens every time
	int printIdx = -1;
	// Occupancy
	uint64_t occ = pieces[6] | pieces[7];
	// Used for sanity checks during debugging (piece set but no occupancy)
	[[maybe_unused]] uint64_t sanity = (pieces[0] | pieces[1] | pieces[2] | pieces[3] | pieces[4] | pieces[5]) ^ occ;
	for (uint16_t rank = RANK_8; rank <= RANK_8; rank--) { // Catches wrap-around subtraction
		for (uint16_t file = FILE_A; file <= FILE_H; file++) {
			uint64_t bits = square_bits((Rank)rank, (File)file);
			printIdx++;

#ifndef NODEBUG
			if (sanity & bits) { // Occupancy and collective piece boards differ
				if (occ & bits) // Occupied but no piece specified
					print[printIdx] = '?';
				else // Piece specified but no occupancy
					print[printIdx] = '!';
				continue;
			}
#endif

			if (!(occ & bits)) {
				print[printIdx] = '.';
				continue;
			}

#ifndef NODEBUG
			bool put = false;
#endif

			// Scan through piece types trying to find one that matches
			for (int type = PAWN; type < NO_PIECE; type++) {
				if (!(pieces[type] & bits))
					continue;

#ifndef NODEBUG
				if (put) { // More than two pieces on one square
					print[printIdx] = '&';
					break;
				}
				put = true;
#endif

				print[printIdx] = piece_letter[type] + (!!(pieces[7] & bits) << 5);
			}
		}
	}
	// Print the board
	for (int i = 0; i < 64; i++) {
		std::cout << print[i];
		if (i % 8 == 7)
			std::cout << std::endl;
		else
			std::cout << ' ';
	}
}

bool Board::make_move(Move) {
	/// TODO: IMPLEMENT
}

void Board::unmake_move() {
	/// TODO: IMPLEMENT
}

void Board::legal_moves(std::vector<Move> &moves) const {
	/// TODO: IMPLEMENT
}

uint64_t Board::hash() const {
	uint64_t hash = 0;
	for (int i = 0; i < 6; i++) {
		hash ^= pieces[i];
	}
	hash ^= castling;
	hash ^= ep_square;
	hash ^= control[0];
	hash ^= control[1];
	hash ^= side << 62;
}