#pragma once

#include "includes.hpp"

constexpr uint64_t FileABits = 0x0101010101010101ULL;
constexpr uint64_t FileBBits = FileABits << 1;
constexpr uint64_t FileCBits = FileABits << 2;
constexpr uint64_t FileDBits = FileABits << 3;
constexpr uint64_t FileEBits = FileABits << 4;
constexpr uint64_t FileFBits = FileABits << 5;
constexpr uint64_t FileGBits = FileABits << 6;
constexpr uint64_t FileHBits = FileABits << 7;

constexpr uint64_t Rank1Bits = 0xff;
constexpr uint64_t Rank2Bits = Rank1Bits << (8 * 1);
constexpr uint64_t Rank3Bits = Rank1Bits << (8 * 2);
constexpr uint64_t Rank4Bits = Rank1Bits << (8 * 3);
constexpr uint64_t Rank5Bits = Rank1Bits << (8 * 4);
constexpr uint64_t Rank6Bits = Rank1Bits << (8 * 5);
constexpr uint64_t Rank7Bits = Rank1Bits << (8 * 6);
constexpr uint64_t Rank8Bits = Rank1Bits << (8 * 7);

extern uint64_t pseudoAttacks[6][64];

// clang-format off
constexpr uint64_t square_bits(Square x) { return 1ULL << x; }
constexpr uint64_t square_bits(Rank r, File f) { return 1ULL << (r * 8 + f); }
// clang-format on

// A move needs 16 bits to be stored
// bits 0-5: Destination square (from 0 to 63)
// bits 6-11: Origin (from 0 to 63)
// bits 12-13: promotion piece type - 1 (from KNIGHT-1 to QUEEN-1)
// bits 14-15: special move flag: promotion (1), en passant (2), castling (3)
// NOTE: en passant bit is only set when a pawn can be captured
struct Move {
	uint16_t data;
	constexpr explicit Move(uint16_t d) : data(d) {}
	constexpr Move(int from, int to) : data((from << 6) | to) {}
	template <MoveType T> static constexpr Move make(Square from, Square to, PieceType pt = KNIGHT) {
		return Move(T | ((pt - KNIGHT) << 12) | (from << 6) | to);
	}
	constexpr int src() {
		return data >> 6 & 0x3f;
	};
	constexpr int dst() {
		return data & 0x3f;
	};
	constexpr bool operator==(const Move &m) const {
		return data == m.data;
	};
	constexpr bool operator!=(const Move &m) const {
		return data == m.data;
	};
};

struct Board {
	// pawns, knights, bishops, rooks, queens, kings, w_occupancy, b_occupancy
	uint64_t pieces[8] = {0};
	// w_control, b_control
	uint64_t control[2] = {0};
	bool side = WHITE;
	uint8_t castling = 0xf;
	Square ep_square = SQ_NONE;

	// Moves with extra information (taken piece etc..)
	// better documentation will be included later
	std::stack<uint32_t> move_hist;
	std::stack<uint16_t> meta_hist;

	Board() {
		// Load starting position
		pieces[0] = Rank2Bits | Rank7Bits;
		pieces[1] = square_bits(SQ_B1) | square_bits(SQ_G1) | square_bits(SQ_B8) | square_bits(SQ_G8);
		pieces[2] = square_bits(SQ_C1) | square_bits(SQ_F1) | square_bits(SQ_C8) | square_bits(SQ_F8);
		pieces[3] = square_bits(SQ_A1) | square_bits(SQ_H1) | square_bits(SQ_A8) | square_bits(SQ_H8);
		pieces[4] = square_bits(SQ_D1) | square_bits(SQ_D8);
		pieces[5] = square_bits(SQ_E1) | square_bits(SQ_E8);
		pieces[6] = Rank1Bits | Rank2Bits;
		pieces[7] = Rank7Bits | Rank8Bits;
	}
	Board(std::string fen) {
		load_fen(fen);
	};

	void load_fen(std::string);
	void print_board();

	bool make_move(Move);
	void unmake_move();

	std::vector<Move> legal_moves();
};
