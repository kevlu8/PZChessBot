#include "search.hpp"

int count = 0;

std::unordered_map<unsigned long long, std::pair<std::string, int> *> hashtable;

unsigned long long hash(const char *board, const char *metadata) {
	// Zobrist hashing
	unsigned long long hash = 0;
	for (int i = 0; i < 64; i++) {
		if (board[i] == 0)
			continue;
		hash ^= zobrist[(board[i] - 1) * 64 + i];
	}
	return hash;
}

void order_moves(const std::vector<std::string> *moves, const char *board, const std::string prevmove, const char *metadata, const char *control, const char *opp_control, const bool turn, std::set<std::pair<int, std::string>> *ordered_moves) {
	char newboard[64], newmeta[3];
	int tmp, tmp2;
	for (std::string move : *moves) {
		memcpy(newboard, board, 64);
		memcpy(newmeta, metadata, 3);
		make_move(move, prevmove, newboard, newmeta);
		ordered_moves->insert({(turn ? -1 : 1) * eval(newboard, control, opp_control, nullptr, nullptr), move});
	}
}

std::pair<std::string, int> *__recurse(int depth, int maxdepth, const char *board, const std::string prevmove, const char *metadata, const char *prev_w_control, const char *prev_b_control, const bool turn, int alpha = -1e9 - 5, int beta = 1e9 + 5) noexcept {
	std::vector<std::string> *moves = find_legal_moves(board, prevmove, metadata);
	std::set<std::pair<int, std::string>> *ordered_moves = new std::set<std::pair<int, std::string>>;
	order_moves(moves, board, prevmove, metadata, turn ? prev_w_control : prev_b_control, turn ? prev_b_control : prev_w_control, turn, ordered_moves);
	// for (const std::pair<int, std::string> curr : *ordered_moves) {
	// 	std::cout << curr.first << " " << curr.second << std::endl;
	// }
	delete moves;
	std::pair<std::string, int> *bestmove = new std::pair<std::string, int>;
	bestmove->first = "resign";
	bestmove->second = turn ? -1e9 : 1e9;
	std::pair<std::string, int> *move;
	char newboard[64], newmeta[3], w_control[64], b_control[64];
	int w_king_pos = -1, b_king_pos = -1;
	for (int i = 0; i < 64; i++) {
		if (board[i] == 6)
			w_king_pos = i;
		else if (board[i] == 12)
			b_king_pos = i;
	}
	int i = 0;
	for (const std::pair<int, std::string> curr : *ordered_moves) {
		memcpy(newboard, board, 64);
		memcpy(newmeta, metadata, 3);
		make_move(curr.second, prevmove, newboard, newmeta);
		controlled_squares(newboard, true, w_control, false);
		controlled_squares(newboard, false, b_control, false);
		int ceval = curr.first * (turn ? -1 : 1);
		if (is_check(turn ? b_control : w_control, turn ? w_king_pos : b_king_pos)) // broken
			continue;
		if (depth == maxdepth) {
			count++;
			move = new std::pair<std::string, int>;
			move->first = curr.second;
			move->second = ceval;
		} else {
			move = __recurse(depth + 1, std::max(depth + 1, maxdepth - i), newboard, curr.second, newmeta, w_control, b_control, !turn, alpha, beta);
			move->first = curr.second;
		}
		if (turn) {
			if (move->second > bestmove->second) {
				bestmove->first = move->first;
				bestmove->second = move->second;
			}
			alpha = std::max(alpha, move->second);
		} else {
			if (move->second < bestmove->second) {
				bestmove->first = move->first;
				bestmove->second = move->second;
			}
			beta = std::min(beta, move->second);
		}
		delete move;

		if (beta <= alpha)
			break;
		i++;
	}
	if (bestmove->first == "resign" && !is_check(turn ? b_control : w_control, turn ? w_king_pos : b_king_pos)) {
		bestmove->first = "stalemate";
		bestmove->second = 0;
	}
	delete ordered_moves;
	return bestmove;
}

const std::string ab_search(const char *position, const int depth, const char *metadata, const std::string prev, const bool turn) noexcept {
	char w_control[64], b_control[64];
	controlled_squares(position, true, w_control, false);
	controlled_squares(position, false, b_control, false);
	std::pair<std::string, int> *ret = __recurse(1, depth, position, prev, metadata, w_control, b_control, turn);
	std::cout << "explored " << count << " nodes\n";
	std::string ans = std::string(ret->first);
	delete ret;
	return ans;
}
