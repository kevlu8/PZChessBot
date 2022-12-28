#include "search.hpp"

#define INF (1e9 + 100)

unsigned long long count = 0;

// 1e9 + 100 is the number that should be used as default values, they are never achievable naturally (unless maxdepth exceeds 99)
// 1e9 should be used in the case of a checkmate

std::pair<int, uint16_t> __recurse(Board &b, const int depth, const int target, int alpha, int beta) {
	std::unordered_set<uint16_t> moves;
	b.legal_moves(moves);
	std::vector<std::pair<int, uint16_t>> container;
	container.reserve(moves.size());
	for (auto &m : moves) {
		b.make_move(m);
		container.push_back({((b.side() << 1) - 1) * b.eval(), m});
		b.unmake_move();
	}
	std::priority_queue<std::pair<int, uint16_t>> orderedmoves(container.begin(), container.end());
	std::pair<int, uint16_t> bestmove = {((b.side() << 1) - 1) * -INF, 0};
	std::pair<int, uint16_t> move;
	while (!orderedmoves.empty()) {
		count++;
		move = orderedmoves.top();
		orderedmoves.pop();
		b.make_move(move.second);
		if (depth == 1) {
			if (abs(move.first - target) < abs(bestmove.first - target))
				bestmove = move;
			bestmove = std::max(bestmove, {move.first, move.second});
		} else {
			bestmove = std::max(bestmove, __recurse(b, depth - 1, -target, -beta, -alpha));
			bestmove.first += ((bestmove.first >> 30) & 0b10) - 1; // decrease magnitude of positions that take longer to get to (this should hopefully make the engine prefer faster mates while stalling out getting mated)
			if (abs(move.first - target) < abs(bestmove.first - target))
				bestmove = move;
		}
		b.unmake_move();
		if (b.side())
			alpha = std::max(alpha, bestmove.first);
		else
			beta = std::min(beta, bestmove.first);
		// if (beta <= alpha) {
		// 	break;
		// }
	}
	return bestmove;
}

uint16_t ab_search(Board &b, const int depth) {
	uint16_t ans = __recurse(b, depth, ((b.side() << 1) - 1) * INF, -INF, INF).second;
	std::cout << count << std::endl;
	return ans;
}
