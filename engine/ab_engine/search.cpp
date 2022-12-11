#include "search.hpp"

std::vector<std::pair<std::string, int>> __recurse(int depth, int maxdepth, const char *board, const std::string prevmove, const char *metadata, const bool turn, const int target, int alpha = -1e9 - 5, int beta = 1e9 + 5) noexcept {
	std::vector<std::string> moves = find_legal_moves(board, prevmove, metadata);
	std::vector<std::pair<std::string, int>> bestmove = {{"resign", target > 0 ? -1e9 : 1e9}};
	std::vector<std::pair<std::string, int>> move;
	char newboard[64];
	char newmeta[3];
	if (turn) {
		int best = -1e9 - 5;
		for (std::string curr : moves) {
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);
			if (is_check(newboard, turn))
				continue;
			if (check_mate(newboard, !turn, prevmove, newmeta))
				return {{curr, target > 0 ? 1e9 + 5 : -1e9 - 5}};
			int aeval = eval(newboard, newmeta, curr);
			if (depth == maxdepth) {
				move = {{curr, aeval}};
			} else {
				move = __recurse(depth + 1, maxdepth, newboard, curr, newmeta, !turn, -target, alpha, beta);
				move.insert(move.begin(), {curr, move[0].second});
				if (move[0].second == (turn ? 1e9 : -1e9)) {
					return move;
				}
			}
			if (abs(move[0].second - target) < abs(bestmove[0].second - target))
				bestmove = move;
			best = std::max(best, move[0].second);
			alpha = std::max(alpha, best);
			if (beta <= alpha)
				break;
		}
	} else {
		int best = 1e9 + 5;
		for (std::string curr : moves) {
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);
			if (is_check(newboard, turn))
				continue;
			if (check_mate(newboard, !turn, prevmove, newmeta))
				return {{curr, target > 0 ? 1e9 : -1e9}};
			int beval = eval(newboard, newmeta, curr);
			if (depth == maxdepth) {
				move = {{curr, beval}};
			} else {
				move = __recurse(depth + 1, maxdepth, newboard, curr, newmeta, !turn, -target, alpha, beta);
				move.insert(move.begin(), {curr, move[0].second});
				// if (move[0].second == (turn ? 1e9 : -1e9)) {
				// 	return move;
				// }
			}
			if (abs(move[0].second - target) < abs(bestmove[0].second - target))
				bestmove = move; // if move has better eval than bestmove, replace bestmove with move
			best = std::min(best, move[0].second);
			beta = std::min(beta, best);
			if (beta <= alpha)
				break;
		}
	}
	return bestmove;
}

std::vector<std::pair<std::string, int>> ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) noexcept {
	return __recurse(1, depth, position, prev, metadata, turn, turn ? 1e9 + 10 : -1e9 - 10);
}
