#include "search.hpp"

#define INF (1e9 + 100)

unsigned long long count = 0, total = 0, checks = 0;

// 1e9 + 100 is the number that should be used as default values, they are never achievable naturally (unless maxdepth exceeds 99)
// 1e9 should be used in the case of a checkmate

std::vector<uint16_t> prev{0};

std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta) {
	std::unordered_set<uint16_t> moves;
	b.legal_moves(moves);
	std::priority_queue<std::pair<int, uint16_t>> orderedmoves;
	for (auto &m : moves) {
		b.make_move(m);
		orderedmoves.push({((b.side() << 1) - 1) * -b.eval(), m});
		b.unmake_move();
	}
	std::pair<int, uint16_t> move, bestmove = {-INF, 0};
	while (!orderedmoves.empty()) {
		move = orderedmoves.top();
		orderedmoves.pop();
		b.make_move(move.second);
		b.controlled_squares(b.side());
		if (!b.in_check(!b.side())) {
			if (depth == 1) {
				count++;
				if (move.first > bestmove.first)
					bestmove = move;
				else if (move.first == bestmove.first && move.second > bestmove.second)
					bestmove = move;
			} else {
				// b.print_board();
				// std::cout << serialize_move(move.second) << ": " << count << std::endl;
				// std::unordered_set<uint16_t> tmp;
				// std::set<std::string> tmp2;
				// b.legal_moves(tmp);
				// for (auto &m : tmp) {
				// 	b.make_move(m);
				// 	if (!b.in_check(!b.side()))
				// 		tmp2.insert(serialize_move(m));
				// 	b.unmake_move();
				// }
				// for (auto &m : tmp2)
				// 	std::cout << m << std::endl;
				prev.push_back(move.second);
				int tmp = -__recurse(b, depth - 1, alpha, beta).first - 1;
				tmp += ((tmp >> 30) & 0b10) - 1; // decrease magnitude of positions that take longer to get to (this should hopefully make the engine prefer faster results while stalling out getting into bad positions)
				prev.pop_back();
				if (tmp > bestmove.first) {
					bestmove = {tmp, move.second};
				} else if (tmp == bestmove.first && move.second > bestmove.second) {
					bestmove = {tmp, move.second};
				}
			}
			total += count;
			count = 0;
			b.unmake_move();
			if (b.side())
				alpha = std::max(alpha, bestmove.first);
			else
				beta = std::min(beta, bestmove.first);
			// if (beta <= alpha)
			// 	break;
		} else
			b.unmake_move();
	}
	if (bestmove.second == 0) {
		b.controlled_squares(!b.side());
		if (b.in_check(b.side()))
			bestmove.first = -1e9;
		else
			bestmove.first = 0;
	}
	return bestmove;
}

uint16_t ab_search(Board &b, const int depth) {
	total = 0;
	std::pair<int, uint16_t> ans = __recurse(b, depth, -INF, INF);
	total += count;
	// std::cout << "Total nodes: " << total << std::endl;
	// std::cout << "eval: " << ans.first << std::endl;
  std::cout << total << std::endl;
	return ans.second;
}
