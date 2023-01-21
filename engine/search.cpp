#include "search.hpp"

#define INF (1e9 + 100)

unsigned long long count = 0, checks = 0;

// 1e9 + 100 is the number that should be used as default values, they are never achievable naturally (unless maxdepth exceeds 99)
// 1e9 should be used in the case of a checkmate

std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta) {
	if (!QUIESCENCE)
		if (depth <= 0) {
			count++;
			return {((b.side() << 1) - 1) * b.eval(), 0};
		}
	uint8_t loud = 0;
	std::unordered_set<uint16_t> moves;
	b.legal_moves(moves);
	std::priority_queue<std::pair<int, uint16_t>> orderedmoves;
	for (auto &m : moves) {
		if (depth <= 0 && QUIESCENCE && m >> 14 == 0)
			continue;
		b.make_move(m);
		orderedmoves.push({((b.side() << 1) - 1) * -b.eval(), m});
		b.unmake_move();
		loud |= m >> 14;
	}
	if (QUIESCENCE)
		if (depth <= 0 && !loud) {
			count++;
			return {((b.side() << 1) - 1) * b.eval(), 0};
		}
	std::pair<int, uint16_t> move, bestmove = {-INF, 0};
	int i = 0;
	while (!orderedmoves.empty()) {
		move = orderedmoves.top();
		orderedmoves.pop();
		b.make_move(move.second);
		b.controlled_squares(b.side());
		if (!b.in_check(!b.side())) {
			int tmp;
			if (NOPRUNE /* || _popcnt64(b.occupied()) < 10*/)
				tmp = -__recurse(b, depth - 1, alpha, beta).first;
			else
				tmp = -__recurse(b, depth - 33 + _lzcnt_u32(i), alpha, beta).first - 1; // decrease magnitude of positions that take longer to get to (this should hopefully make the engine prefer faster results while stalling out getting into bad positions)
			if (tmp > bestmove.first) {
				bestmove = {tmp, move.second};
			} else if (tmp == bestmove.first && move.second > bestmove.second) {
				bestmove = {tmp, move.second};
			}
			b.unmake_move();
			if (b.side())
				alpha = std::max(alpha, bestmove.first);
			else
				beta = std::min(beta, -bestmove.first);
			if (!NOPRUNE) {
				if (beta <= alpha)
					break;
			}
		} else
			b.unmake_move();
		i++;
	}
	if (bestmove.second == 0) {
		b.controlled_squares(!b.side());
		if (b.in_check(b.side()))
			bestmove.first = -1e9;
		else {
			bestmove.first = 0;
			bestmove.second = -1;
		}
	}
	return bestmove;
}

std::pair<int, uint16_t> ab_search(Board &b, const int depth) {
	count = 0;
	std::pair<int, uint16_t> ans = __recurse(b, depth, -INF, INF);
	if (!b.side())
		ans.first = -ans.first;
	return ans;
}

const unsigned long long nodes() { return count; }
