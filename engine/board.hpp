#include <iostream>
#include <string.h>
#include <string>
#include <unordered_set>
#include <utility>

#define STARTING_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

class Board {
private:
public:
	char data[8][8], meta[5], prevmeta[5];
	char prev[7], prevprev[7]; // [move in uci notation][piece on destination square (e for en passant captures)] e.g. e5f6e
	// uint64_t pieces[12] = {0};
	void load_fen(const std::string &);

	Board();
	Board(const std::string &);

	void print_board();
	void make_move(const char *);
	void unmake_move();
	void legal_moves(std::unordered_set<std::string> &);
	void bit_moves(uint64_t[], uint64_t, std::unordered_set<std::string> &);
};
