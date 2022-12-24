#include <iostream>
#include <set>
#include <string.h>
#include <string>
#include <unordered_set>
#include <x86intrin.h>

#include "zobrist"

#define U64 uint64_t
#define C64(x) x##ULL
#define xyx(x, y) ((x) + (y)*8)
#define BIT(x) (C64(1) << (x))
#define BITxy(x, y) (C64(1) << xyx(x, y))
#define LSONES(x) (((C64(1) << (x)) - 1) - ((x) == 64))
#define MSONES(x) ((C64(-1) << (64 - (x))) + ((x) == 0))
#define EXPANDLSB(x) LSONES(__builtin_ia32_tzcnt_u64(x) + 1 - ((x) == 0))
#define EXPANDMSB(x) MSONES(__builtin_ia32_lzcnt_u64(x) + 1 - ((x) == 0))

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

constexpr int fen_to_piece[] = {3, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1, 4, -1, 5, 1, 2};
constexpr char piece_to_fen[] = {'K', 'Q', 'R', 'B', 'N', 'P'};
constexpr int piece_values[] = {0, 100, 320, 330, 500, 900, 20000};

void print_bits(U64);

class Board {
private:
	U64 pieces[8]; // kings, queens, rooks, bishops, knights, pawns, all white, all black
	uint8_t meta[5], prevmeta[5]; // 0: side to move, 1: castling rights, 2: ep square, 3: halfmove clock, 4: fullmove number
	uint32_t prev, prevprev; // previous move, previous previous move [6 bits src][6 bits dst][2 bits promotion piece][2 bits promotion flag][1 byte padding][1 byte piece on dest]
	void load_fen(const std::string &); // load a fen string

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

	bool in_check(const bool);

public:
	Board(); // default constructor
	Board(const std::string &);
	void print_board();
	// [6 bits src][6 bits dst][2 bits promotion piece][2 bits promotion flag]
	void make_move(uint16_t);
	void unmake_move();
	void legal_moves(std::unordered_set<uint16_t> &);
	U64 controlled_squares(const bool side);

	U64 zobrist_hash();
};
