#include "search.hpp"

std::vector<std::pair<std::string, int>> __recurse(int depth, int maxdepth, const char *board, const std::string prevmove, const char *metadata, const bool turn, const int target, int alpha = -1e9 - 5, int beta = 1e9 + 5) {
	std::vector<std::string> moves = find_legal_moves(board, prevmove, metadata);
	std::vector<std::pair<std::string, int>> bestmove = {{"resign", target > 0 ? -1e9 - 5 : 1e9 + 5}};
	std::vector<std::pair<std::string, int>> move;
	char newboard[64];
	char newmeta[3];
	if (turn) {
		int best = -1e9 - 5;
		for (std::string curr : moves) {
			// if (curr == "a6a7") {
			// 	std::cout << "h5f7" << '\n';
			// }
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);

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
			if (check_mate(newboard, !turn, prevmove, newmeta))
				return {{curr, target > 0 ? 1e9 : -1e9}};
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
			best = std::max(best, aeval);
			alpha = std::max(alpha, best);
			if (beta <= alpha)
				break;
		}
	} else {
		int best = 1e9 + 5;
		for (std::string curr : moves) {
			// if (curr == "g3a3" && depth == 1 && prevmove == "f8a8") {
			// 	std::cout << "h5f7" << '\n';
			// }
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);

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
			int beval = eval(newboard, newmeta, curr);
			if (check_mate(newboard, !turn, prevmove, newmeta))
				return {{curr, target > 0 ? 1e9 : -1e9}};
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
				bestmove = move;
			best = std::min(best, beval);
			beta = std::min(beta, best);
			if (beta <= alpha)
				break;
		}
	}
	return bestmove;
}

std::vector<std::pair<std::string, int>> ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) {
	int aeval = eval(position, metadata, prev);
	std::vector<std::pair<std::string, int>> bestmove = __recurse(1, depth, position, prev, metadata, turn, turn ? 1e9 + 10 : -1e9 - 10);
	return bestmove;
}
