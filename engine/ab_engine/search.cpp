#include "search.hpp"

std::pair<std::string, int> __recurse(int depth, int maxdepth, const char *board, const std::string currmove, const char *metadata, const bool turn, const int target) {
	std::vector<std::string> moves = find_legal_moves(board, currmove, metadata);
	std::pair<std::string, int> bestmove = {"resign", target > 0 ? INT32_MIN : INT32_MAX};
	std::pair<std::string, int> move;
	char newboard[64];
	char newmeta[3];
	for (std::string move : moves) {
		memcpy(newboard, board, 64);
		memcpy(newmeta, metadata, 3);
		make_move(move, newboard, newmeta);
		if (is_check(newboard, turn))
			continue;
		move = __recurse(depth + 1, maxdepth, newboard, move, newmeta, !turn, target);
		if (abs(move.second - target) < abs(bestmove.second - target))
			bestmove = move;
	}
}

std::pair<std::string, int> ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) {
	int aeval = eval(position, metadata, prev);
	std::pair<std::string, int> bestmove = __recurse(0, depth, position, prev, metadata, turn, turn ? INT32_MAX : INT32_MIN);
	return bestmove;
}
