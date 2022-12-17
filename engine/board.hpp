#pragma once

#include <stdlib.h>
#include <memory.h>

class Board {
private:
	char *prev, board[8][8], *meta; // prev is only used internally so you can use internal flags: e.g. h2Pg3Q for pawn on h2 takes queen on g3
	char w_control[8][8], b_control[8][8];
	int w_king_pos = -1, b_king_pos = -1;
public:
	Board();

	Board(char[8][8], const char *) noexcept;

	~Board() noexcept;

	void make_move(const char *) noexcept;

	void unmake_move() noexcept;

	int eval() noexcept;

	inline bool is_check() { return (meta[0] ? b_control : w_control)[meta[0] ? b_king_pos : w_king_pos] != 0; };

	bool is_checkmate() noexcept;
};