#pragma once

#include <iostream>
#include <set>
#include <string.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <x86intrin.h>

#include "zobrist"

#define U64 uint64_t
#define C64(x) x##ULL
#define xyx(x, y) ((x) + (y)*8)
#define BIT(x) (C64(1) << (x))
#define BITxy(x, y) (C64(1) << xyx(x, y))
#define LSONES(x) (((C64(1) << (x)) - 1) - ((x) == 64))
#define MSONES(x) ((C64(-1) << (64 - (x))) + ((x) == 0))
#define EXPANDLSB(x) LSONES(_tzcnt_u64(x) + 1 - ((x) == 0))
#define EXPANDMSB(x) MSONES(_lzcnt_u64(x) + 1 - ((x) == 0))

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

constexpr int fen_to_piece[] = {3, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 4, -1, 5, 1, 2};
constexpr char piece_to_fen[] = {'K', 'Q', 'R', 'B', 'N', 'P'};
constexpr int piece_values[] = {20000, 900, 500, 330, 320, 100};

void print_bits(U64);

class Board {
private:
	U64 pieces[8]; // kings, queens, rooks, bishops, knights, pawns, all white, all black
	uint8_t *meta; // 0: side to move, 1: castling rights, 2: ep square, 3: halfmove clock, 4: fullmove number
	std::vector<uint8_t *> meta_hist;
	std::vector<uint32_t> move_hist; // previous moves [6 bits src][6 bits dst][2 bits miscellaneous][1 bit capture flag][1 bit promotion flag][1 piece that moved][1 byte piece on dest]

	std::unordered_map<U64, int> pos_hist;

	U64 hash;
	U64 control[2]; // black, white

	U64 king_control(const bool);
	U64 rook_control(const bool);
	U64 bishop_control(const bool);
	U64 knight_control(const bool);
	U64 white_pawn_control();
	U64 black_pawn_control();

	void king_moves(std::unordered_set<uint16_t> &);
	void rook_moves(std::unordered_set<uint16_t> &);
	void bishop_moves(std::unordered_set<uint16_t> &);
	void knight_moves(std::unordered_set<uint16_t> &);
	void white_pawn_moves(std::unordered_set<uint16_t> &);
	void black_pawn_moves(std::unordered_set<uint16_t> &);

	U64 zobrist_hash();

	const inline constexpr bool insufficient() const { return !(pieces[1] | pieces[2] | pieces[5]) && __builtin_popcountll(pieces[3] | pieces[4]) <= 1; }

public:
	Board(); // default constructor
	Board(const std::string &);
	void load_fen(const std::string &); // load a fen string
	void print_board();
	// [6 bits src][6 bits dst][2 bits miscellaneous][1 bit capture flag][1 bit promotion flag]
	void make_move(uint16_t);
	void unmake_move();
	void legal_moves(std::unordered_set<uint16_t> &);
	const inline constexpr bool side() const { return meta[0]; }
	const inline constexpr U64 occupied() const { return pieces[6] | pieces[7]; }
	const inline constexpr U64 kings() const { return pieces[0]; }
	const inline constexpr U64 queens() const { return pieces[1]; }
	const inline constexpr U64 rooks() const { return pieces[2]; }
	const inline constexpr U64 bishops() const { return pieces[3]; }
	const inline constexpr U64 knights() const { return pieces[4]; }
	const inline constexpr U64 pawns() const { return pieces[5]; }

	bool in_check(const bool);

	void controlled_squares(const bool side);

	int eval();

	const inline constexpr U64 currhash() const { return hash; };

	const inline bool ended() { return pos_hist[hash] >= 3 || insufficient() || meta[3] >= 100; }
};

std::string stringify_move(uint16_t);
uint16_t parse_move(Board &, std::string);
