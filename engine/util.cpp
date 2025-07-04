#include "util.hpp"

std::string score_to_string(Value score) {
	if (abs(score) >= VALUE_MATE_MAX_PLY) {
		int moves = (VALUE_MATE - abs(score)) / 2;
		if (score < 0) moves *= -1;
		return "mate " + std::to_string(moves);
	}
	return std::to_string(score);
}

bool is_capture(Move &m, Board &board) {
	return board.mailbox[m.dst()] != NO_PIECE || m.type() == EN_PASSANT;
	// We don't need to check the side of the piece because we shouldn't be capturing our own pieces
}