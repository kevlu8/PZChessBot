#pragma once

#include "zobrist"
#include <memory.h>
#include <stdlib.h>

class Board {
private:
	// prev is only used internally so you can use internal flags: e.g. h2g3pQ for pawn on h2 takes queen on g3
	// prev format is like this "[move][piece on starting square][piece on ending square]"
	// the move is in uci format (e.g. e2e4)
	// the piece on starting square is the piece that was on the starting square before the move (capital letter for white, lowercase for black)
	// the piece on ending square is the piece that was on the ending square before the move (capital letter for white, lowercase for black, - for empty)
	char *prev, *prevmeta, board[8][8], *meta;
	char w_control[8][8], b_control[8][8];
	int w_king_pos = -1, b_king_pos = -1;

public:
	Board();
	Board(char[8][8], const char *) noexcept;
	~Board() noexcept;
	void make_move(const char *, unsigned long long &) noexcept;
	void unmake_move() noexcept;
	int eval() noexcept;
	inline bool is_check() { return (meta[0] ? b_control : w_control)[meta[0] ? b_king_pos : w_king_pos] != 0; };
	bool is_checkmate() noexcept;
};