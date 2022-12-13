#include "search.hpp"
int count = 0;

std::pair<std::string, int> *__recurse(int depth, int maxdepth, const char *board, const std::string prevmove, const char *metadata, const bool turn, int alpha = -1e9 - 5, int beta = 1e9 + 5) noexcept {
	std::vector<std::string> *moves = find_legal_moves(board, prevmove, metadata);
	count++;
	std::pair<std::string, int> *bestmove = new std::pair<std::string, int>;
	bestmove->first = "resign";
	bestmove->second = turn ? -1e9 : 1e9;
	std::pair<std::string, int> *move;
	char newboard[64];
	char newmeta[3];
	char w_control[64];
	char b_control[64];
	int w_king_pos = -1;
	int b_king_pos = -1;
	if (turn) {
		for (const std::string curr : *moves) {
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);
			controlled_squares(newboard, true, w_control, false);
			controlled_squares(newboard, false, b_control, false);
			int aeval = eval(newboard, newmeta, curr, w_control, b_control, w_king_pos, b_king_pos);
			if (is_check(b_control, w_king_pos))
				continue;
			if (depth == maxdepth) {
				move = new std::pair<std::string, int>;
				move->first = curr;
				move->second = aeval;
			} else {
				move = __recurse(depth + 1, maxdepth, newboard, curr, newmeta, !turn, alpha, beta);
				move->first = curr;
			}
			if (move->second > bestmove->second) {
				bestmove->first = move->first;
				bestmove->second = move->second;
			}
			alpha = std::max(alpha, move->second);
			delete move;
			if (beta <= alpha)
				break;
		}
	} else {
		for (const std::string curr : *moves) {
			memcpy(newboard, board, 64);
			memcpy(newmeta, metadata, 3);
			make_move(curr, prevmove, newboard, newmeta);
			controlled_squares(newboard, true, w_control, false);
			controlled_squares(newboard, false, b_control, false);
			int beval = eval(newboard, newmeta, curr, w_control, b_control, w_king_pos, b_king_pos);
			if (is_check(w_control, b_king_pos))
				continue;
			if (depth == maxdepth) {
				move = new std::pair<std::string, int>;
				move->first = curr;
				move->second = beval;
			} else {
				move = __recurse(depth + 1, maxdepth, newboard, curr, newmeta, !turn, alpha, beta);
				move->first = curr;
			}
			if (move->second <= bestmove->second) {
				bestmove->first = move->first;
				bestmove->second = move->second;
			}
			alpha = std::max(alpha, move->second);
			delete move;
			if (beta <= alpha)
				break;
		}
	}
	delete moves;
	return bestmove;
}

const std::string &ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) noexcept {
	std::pair<std::string, int> *ret = __recurse(1, depth, position, prev, metadata, turn);
	std::string &ans = *new std::string(ret->first);
	delete ret;
	std::cout << "explored " << count << " nodes\n";
	return ans;
}
