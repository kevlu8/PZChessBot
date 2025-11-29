#pragma once

#include "includes.hpp"
#include "move.hpp"
#include "ttable.hpp"
#include "boardstate.hpp"

// Selects the occupancy array by xoring 6 with side (white: false = 0 ^ 6 = 6, black: true = 1 ^ 6 = 7)
#define OCC(side) (6 ^ (side))
#define OPPOCC(side) (7 ^ (side))

constexpr Bitboard FileABits = 0x0101010101010101ULL;
constexpr Bitboard FileBBits = FileABits << 1;
constexpr Bitboard FileCBits = FileABits << 2;
constexpr Bitboard FileDBits = FileABits << 3;
constexpr Bitboard FileEBits = FileABits << 4;
constexpr Bitboard FileFBits = FileABits << 5;
constexpr Bitboard FileGBits = FileABits << 6;
constexpr Bitboard FileHBits = FileABits << 7;

constexpr Bitboard Rank1Bits = 0xff;
constexpr Bitboard Rank2Bits = Rank1Bits << (8 * 1);
constexpr Bitboard Rank3Bits = Rank1Bits << (8 * 2);
constexpr Bitboard Rank4Bits = Rank1Bits << (8 * 3);
constexpr Bitboard Rank5Bits = Rank1Bits << (8 * 4);
constexpr Bitboard Rank6Bits = Rank1Bits << (8 * 5);
constexpr Bitboard Rank7Bits = Rank1Bits << (8 * 6);
constexpr Bitboard Rank8Bits = Rank1Bits << (8 * 7);

extern Bitboard pseudoAttacks[6][64];

// clang-format off
constexpr Bitboard square_bits(Square x) { return 1ULL << x; }
constexpr Bitboard square_bits(Rank r, File f) { return 1ULL << (r * 8 + f); }
// clang-format on

void print_bitboard(Bitboard);

// A history entry stores the move and other information needed to unmake it
// bits 0-15: move
// bits 16-19: captured piece
// bits 20-23: previous castling rights
// bits 24-29: previous en passant square
struct HistoryEntry {
	uint32_t data;
	constexpr explicit HistoryEntry(uint32_t d) : data(d) {}
	constexpr HistoryEntry(Move m, Piece prev_piece, uint8_t prev_castling, Square prev_ep)
		: data(m.data | ((uint32_t)prev_piece << 16) | ((uint32_t)prev_castling << 20) | ((uint32_t)prev_ep << 24)) {}
	constexpr Move move() {
		return Move(data & 0xffff);
	}
	constexpr Piece prev_piece() {
		return Piece((data >> 16) & 0b1111);
	}
	constexpr uint8_t prev_castling() {
		return (data >> 20) & 0b1111;
	}
	constexpr Square prev_ep() {
		return Square(data >> 24);
	}
};

struct Board {
	Bitboard piece_boards[8] = {0};
	bool side = WHITE;
	uint8_t halfmove = 0;
	uint8_t castling = 0xf; // 1111
	Square ep_square = SQ_NONE;
	uint64_t zobrist = 0;
	uint64_t pawn_hash = 0;
	uint64_t nonpawn_hashval[2] = {0, 0}; // [side]
	uint64_t major_hash = 0, minor_hash = 0;
	pzstd::largevector<uint64_t> hash_hist;

	// Mailbox representation of the board for faster queries of certain data
	Piece mailbox[8 * 8];

	// Moves with extra information (taken piece etc..)
	// better documentation will be included later
	std::stack<HistoryEntry> move_hist;
	std::stack<uint8_t> halfmove_hist;

	Board() {
		reset_board();
		recompute_hash();
	}

	Board(std::string fen) {
		load_fen(fen);
		recompute_hash();
	};

	void load_fen(std::string);
	std::string get_fen() const;
	void print_board() const;
	void print_board_pretty(bool print_meta = false) const;
	bool sanity_check(char *);

	void reset_board();
	void reset_startpos() { reset_board(); }
	void reset(std::string fen) { reset_board(); load_fen(fen); }

	void make_move(Move);
	void unmake_move();

	void legal_moves(pzstd::vector<Move> &) const;
	void captures(pzstd::vector<Move> &) const;
	bool control(int, bool) const;
	Value see_capture(Move);
	Bitboard __lva(Square, int side, PieceType &p, Bitboard occ) const;
	bool is_pseudolegal(Move) const;
	constexpr bool is_capture(Move m) const {
		return (piece_boards[OPPOCC(side)] & square_bits(m.dst()));
	}

	void recompute_hash();

	bool threefold(int ply);
	bool insufficient_material() const;

	uint64_t pawn_struct_hash() const;
	uint64_t nonpawn_hash(bool color) const;
	uint64_t material_hash() const;
};
