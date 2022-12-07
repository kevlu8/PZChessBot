#include "search.hpp"

std::pair<std::string, int> __recurse(int depth, int maxdepth, const char *board, const std::string prevmove, const char *metadata, const bool turn, const int target, int highest = -1e9, int lowest = 1e9) {
	std::vector<std::string> moves = find_legal_moves(board, prevmove, metadata);
	std::pair<std::string, int> bestmove = {"resign", target > 0 ? -1e9 : 1e9};
	std::pair<std::string, int> move;
	char newboard[64];
	char newmeta[3];
	for (std::string curr : moves) {
		memcpy(newboard, board, 64);
		memcpy(newmeta, metadata, 3);
		make_move(curr, prevmove, newboard, newmeta);
		int curreval = eval(newboard, newmeta, curr);
		if (turn) {
			if (curreval > highest) {
				highest = curreval;
				bestmove = {curr, curreval};
				// std::cout << "not pruned: " << curr << " eval: " << curreval << '\n';
			} else {
				// std::cout << "pruned: " << curr << " eval: " << curreval << '\n';
				continue;
			}
		} else {
			if (curreval < lowest) {
				lowest = curreval;
				bestmove = {curr, curreval};
			} else {
				continue;
			}
		}

		// std::cout << "move: "<< curr << '\n';
		// for (int i = 0; i < 64; i++) {
		// 	std::cout << std::setw(3) << (int)newboard[(7 - i / 8) * 8 + i % 8];
		// 	if (i % 8 == 7) {
		// 		std::cout << '\n';
		// 	}
		// }
		// std::cout << '\n';
		if (is_check(newboard, turn))
			continue;
		if (depth == maxdepth) {
			move = {curr, eval(newboard, newmeta, curr)};
		} else {
			move = __recurse(depth + 1, maxdepth, newboard, curr, newmeta, !turn, target, highest, lowest);
		}
		if (abs(move.second - target) < abs(bestmove.second - target))
			bestmove = move;
	}
	return bestmove;
}

std::pair<std::string, int> ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) {
	int aeval = eval(position, metadata, prev);
	std::pair<std::string, int> bestmove = __recurse(0, depth, position, prev, metadata, turn, turn ? 1e9 : -1e9);
	return bestmove;
}
