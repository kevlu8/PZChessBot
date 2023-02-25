#include "search.hpp"

#define INF (1e9 + 100)

unsigned long long count = 0, checks = 0, d = 0;
std::unordered_map<U64, std::pair<std::pair<int, uint16_t>, int>> hashtable;

// 1e9 + 100 is the number that should be used as default values, they are never achievable naturally (unless maxdepth exceeds 99)
// 1e9 should be used in the case of a checkmate

#ifdef PRINTLINE
std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta, std::deque<std::pair<int, uint16_t>> **line) {
#else
std::pair<int, uint16_t> __recurse(Board &b, const int depth, int alpha, int beta) {
#endif
	std::unordered_set<uint16_t> moves;
	b.legal_moves(moves);
	if (hashtable.find(b.currhash()) != hashtable.end()) {
		if (hashtable[b.currhash()].second >= depth) {
			std::pair<int, uint16_t> tmp = hashtable[b.currhash()].first;
			if (moves.find(tmp.second) != moves.end() && tmp.second != 0) {
				b.make_move(tmp.second);
				b.controlled_squares(b.side());
				if (!b.ended() && !b.in_check(!b.side())) {
					b.unmake_move();
#ifdef PRINTLINE
					(*line)->push_front(tmp);
#endif
					return tmp;
				}
				b.unmake_move();
			}
		}
	}
	if (b.ended())
		return {0, 0};
	if (!QUIESCENCE)
		if (depth <= 0) {
			count++;
			std::pair<int, uint16_t> move = {((b.side() << 1) - 1) * b.eval(), 0};
#ifdef PRINTLINE
			delete *line;
			*line = new std::deque<std::pair<int, uint16_t>>{};
#endif
			hashtable[b.currhash()] = {{((b.side() << 1) - 1) * move.first, move.second}, 0};
			return move;
		}
	bool quiet = true;
	std::priority_queue<std::pair<int, uint16_t>> orderedmoves;
	for (auto &m : moves) {
		// ignore quiet moves
		// if (depth <= 0 && (m >> 14) == 0)
		// 	continue;
		// if (m == banned)
		// 	continue;
		b.make_move(m);
		if (hashtable.find(b.currhash()) == hashtable.end())
			hashtable[b.currhash()] = {{b.eval(), 0}, 0};
		orderedmoves.push({((b.side() << 1) - 1) * hashtable[b.currhash()].first.first, m});
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
			hashtable[b.currhash()] = {{((b.side() << 1) - 1) * move.first, move.second}, 0};
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
#ifdef PRINTLINE
			std::deque<std::pair<int, uint16_t>> *tmpline = new std::deque<std::pair<int, uint16_t>>;
#endif
			if (NOPRUNE || __builtin_popcountll(b.occupied()) <= 7)
#ifdef PRINTLINE
				tmp = -__recurse(b, depth - 1, alpha, beta, &tmpline).first;
#else
				tmp = -__recurse(b, depth - 1, alpha, beta).first;
#endif
			else
#ifdef PRINTLINE
				tmp = -__recurse(b, depth - ((depth != d - 1 && i > 28 && depth > 2) ? 1 : 0) - 1, alpha, beta, &tmpline).first;
#else
				tmp = -__recurse(b, depth - ((depth != d - 1 && i > 28 && depth > 2) ? 1 : 0) - 1, alpha, beta).first;
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
#ifdef PRINTLINE
				delete *line;
				*line = tmpline;
				(*line)->push_front(bestmove);
#endif
			}
#ifdef PRINTLINE
			else
				delete tmpline;
#endif
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
			bestmove.second = 0;
		}
	}
	hashtable[b.currhash()] = {{((b.side() << 1) - 1) * bestmove.first, move.second}, depth};
	return bestmove;
}

std::pair<int, uint16_t> ab_search(Board &b, const int depth) {
	count = 0;
	d = depth;
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
