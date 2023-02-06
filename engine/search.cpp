#include "search.hpp"

#define INF (1e9 + 100)

unsigned long long count = 0, checks = 0;

// 1e9 + 100 is the number that should be used as default values, they are never achievable naturally (unless maxdepth exceeds 99)
// 1e9 should be used in the case of a checkmate

#ifdef PRINTLINE
std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta, std::deque<std::pair<int, uint16_t>> **line) {
#else
std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta) {
#endif
	if (!QUIESCENCE)
		if (depth <= 0) {
			count++;
			std::pair<int, uint16_t> move = {((b.side() << 1) - 1) * b.eval(), 0};
#ifdef PRINTLINE
			delete *line;
			*line = new std::deque<std::pair<int, uint16_t>>{};
#endif
			return move;
		}
	bool quiet = true;
	std::unordered_set<uint16_t> moves;
	b.legal_moves(moves);
	std::priority_queue<std::pair<int, uint16_t>> orderedmoves;
	for (auto &m : moves) {
		// ignore quiet moves
		if (depth <= 0 && (m >> 14) == 0)
			continue;
		if (depth <= 0)
			std::cout << "";
		b.make_move(m);
		orderedmoves.push({((b.side() << 1) - 1) * -b.eval(), m});
		b.unmake_move();
		quiet = false;
	}
	if (QUIESCENCE) {
		if (depth <= 0 && quiet) {
			count++;
			std::pair<int, uint16_t> move = {((b.side() << 1) - 1) * b.eval(), 0};
#ifdef PRINTLINE
			delete *line;
			*line = new std::deque<std::pair<int, uint16_t>>{};
#endif
			return move;
		}
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
			std::deque<std::pair<int, uint16_t>> *tmpline = new std::deque<std::pair<int, uint16_t>>;
			if (NOPRUNE || _popcnt64(b.occupied()) <= 7)
#ifdef PRINTLINE
				tmp = -__recurse(b, depth - 1, alpha, beta, &tmpline).first;
#else
				tmp = -__recurse(b, depth - 1, alpha, beta).first;
#endif
			else
#ifdef PRINTLINE
				tmp = -__recurse(b, std::max(depth - ((33 - _lzcnt_u32(i)) >> 1) - 1, (uint32_t)0), alpha, beta, &tmpline).first;
#else
				tmp = -__recurse(b, std::max(depth - ((33 - _lzcnt_u32(i)) >> 1) - 1, (uint32_t)0), alpha, beta).first;
#endif
			tmp += ((tmp >> 30) & 0b10) - 1; // decrease magnitude of positions that take longer to get to (this should hopefully make the engine prefer faster results while stalling out getting into bad positions)
			if (tmp > bestmove.first) {
				bestmove = {tmp, move.second};
#ifdef PRINTLINE
				delete *line;
				*line = tmpline;
				(*line)->push_front(bestmove);
#endif
			} else if (tmp == bestmove.first && move.second > bestmove.second) {
				bestmove = {tmp, move.second};
#ifdef PRINTLINE
				delete *line;
				*line = tmpline;
				(*line)->push_front(bestmove);
#endif
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
			i++;
		} else
			b.unmake_move();
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
#ifdef PRINTLINE
	std::deque<std::pair<int, uint16_t>> *line = new std::deque<std::pair<int, uint16_t>>;
	std::pair<int, uint16_t> ans = __recurse(b, depth, -INF, INF, &line);
#else
	std::pair<int, uint16_t> ans = __recurse(b, depth, -INF, INF);
#endif
	if (!b.side())
		ans.first = -ans.first;
#ifdef PRINTLINE
	for (auto it : *line) {
		std::cout << stringify_move(it.second) << std::endl;
	}
	std::cout << ans.first << std::endl;
#endif
	return ans;
}

const unsigned long long nodes() { return count; }
