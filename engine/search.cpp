#include "search.hpp"
#include <utility>

#define MOVENUM(x) ((((#x)[1] - '1') << 12) | (((#x)[0] - 'a') << 8) | (((#x)[3] - '1') << 4) | ((#x)[2] - 'a'))

uint64_t mx_nodes = 1e18; // Maximum nodes to search
bool stop_search = true;
std::chrono::steady_clock::time_point start;
uint64_t mxtime = 1e18; // Maximum time to search in milliseconds

uint16_t num_threads = 1;

std::atomic<uint64_t> nodecnt[64][64] = {{}};
uint64_t nodes[MAX_THREADS] = {};

uint64_t perft(Board &board, int depth) {
	// If white's turn is beginning and black is in check
	if (board.side == WHITE && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7]), WHITE))
		return 0;
	// If black's turn is beginning and white is in check
	else if (board.side == BLACK && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6]), BLACK))
		return 0;
	if (depth == 0)
		return 1;
	pzstd::vector<Move> moves;
	board.legal_moves(moves);
	uint64_t cnt = 0;
	for (Move &move : moves) {
		board.make_move(move);
		cnt += perft(board, depth - 1);
		board.unmake_move();
	}
	return cnt;
}

/**
 * Determines the amount of depth to reduce the search by, given the move's index and the remaining depth
 *
 * See https://www.chessprogramming.org/Late_Move_Reductions
 */
uint16_t reduction[250][MAX_PLY];

__attribute__((constructor)) void init_lmr() {
	for (int i = 0; i < 250; i++) {
		for (int d = 0; d < MAX_PLY; d++) {
			if (d <= 1 || i <= 1) reduction[i][d] = 1024;
			else reduction[i][d] = (0.77 + log2(i) * log2(d) / 2.36) * 1024;
		}
	}
}

/**
 * MVV_LVA (most valuable victim - least valuable attacker) is a metric for move ordering that helps
 * sort captures and promotions. We basically sort high-value captures first, and low-value captures
 * last.
 * 
 * Currently only used in quiescence search in favor of MVV+CaptHist in the main search
 */
Value MVV_LVA[6][6];

__attribute__((constructor)) void init_mvvlva() {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			if (i == KING)
				MVV_LVA[i][j] = QueenValue * 13 + 1; // Prioritize over all other captures
			else
				MVV_LVA[i][j] = PieceValue[i] * 13 - PieceValue[j];
		}
	}
}

Move next_move(pzstd::vector<std::pair<Move, int>> &scores, int &end) {
	if (end == 0) return NullMove; // Ran out
	Move best_move = NullMove;
	int best_score = -2147483647;
	int idx = 0;
	for (int i = 0; i < end; i++) {
		if (scores[i].second > best_score) {
			best_score = scores[i].second;
			best_move = scores[i].first;
			idx = i;
		}
	}
	swap(scores[idx], scores[--end]);
	return best_move;
}

Value score_to_tt(Value score, int ply) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return score + ply;
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return score - ply;
	} else {
		return score;
	}
}

Value tt_to_score(Value score, int ply) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return score - ply;
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return score + ply;
	} else {
		return score;
	}
}

double get_ttable_sz() {
	int cnt = 0;
	for (int i = 0; i < 1024; i++) {
		if (i >= ttable.TT_SIZE) break;
		if (ttable.TT[i].entries[0].valid()) cnt++;
		if (ttable.TT[i].entries[1].valid()) cnt++;
	}
	return cnt / 2048.0;
}

std::string score_to_uci(Value score) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((VALUE_MATE - score + 1) / 2);
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((-VALUE_MATE - score) / 2);
	} else {
		return "cp " + std::to_string(score);
	}
}

/**
 * Perform the quiescence search
 * 
 * Quiescence search is a technique to avoid the horizon effect, where the evaluation function
 * incorrectly evaluates a position because it is not stable (e.g. there is a hanging queen).
 * In this function, we search only captures and promotions, and return the best score.
 * 
 * TODO:
 * - Search for checks and check evasions (every time I've tried this it has lost tons of elo)
 * - Late move reduction (instead of reducing depth, we reduce the search window) (not a known technique, maybe worth trying?)
 */
Value quiesce(ThreadInfo &ti, Value alpha, Value beta, int side, int depth, bool pv=false) {
	nodes[ti.id]++;

	if (stop_search) return 0;

	if (ti.is_main && !(nodes[ti.id] & 4095)) {
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		if (time > mxtime || nodes[ti.id] > mx_nodes) { // currently, the nodes will be broken but time will be accurate
			stop_search = true;
			return 0;
		}
	}

	if (depth >= MAX_PLY)
		return eval(ti.board, (BoardState *)ti.bs) * side; // Just in case

	TTable::TTEntry *tentry = ttable.probe(ti.board.zobrist);
	Value tteval = 0;
	if (tentry) tteval = tt_to_score(tentry->eval, depth);
	if (!pv && tentry) {
		if (tentry->bound() == EXACT) return tteval;
		if (tentry->bound() == LOWER_BOUND) {
			if (tteval >= beta) return tteval;
		}
		if (tentry->bound() == UPPER_BOUND) {
			if (tteval <= alpha) return tteval;
		}
	}

	Value stand_pat = 0;
	Value raw_eval = 0;
	stand_pat = tentry ? tentry->s_eval : eval(ti.board, (BoardState *)ti.bs) * side;
	raw_eval = stand_pat;
	ti.thread_hist.apply_correction(ti.board, stand_pat);
	if (tentry && abs(tteval) < VALUE_MATE_MAX_PLY && tentry->bound() != (tteval > stand_pat ? UPPER_BOUND : LOWER_BOUND))
		stand_pat = tteval;

	if (!tentry) ttable.store(ti.board.zobrist, -VALUE_INFINITE, raw_eval, 0, NONE, false, NullMove, depth);

	// If it's a mate, stop here since there's no point in searching further
	// Theoretically shouldn't ever happen because of stand pat
	if (stand_pat == VALUE_MATE || stand_pat == -VALUE_MATE)
		return stand_pat;

	// If we are too good, return the score
	if (stand_pat >= beta)
		return stand_pat;
	if (stand_pat > alpha)
		alpha = stand_pat;

	pzstd::vector<Move> moves;
	ti.board.captures(moves);
	if (moves.empty())
		return stand_pat;

	// Sort captures and promotions
	pzstd::vector<std::pair<Move, int>> scores;
	for (Move &move : moves) {
		if (ti.board.piece_boards[OPPOCC(ti.board.side)] & square_bits(move.dst())) {
			int score = 0;
			score = MVV_LVA[ti.board.mailbox[move.dst()] & 7][ti.board.mailbox[move.src()] & 7];
			scores.push_back({move, score});
		} else if (move.type() == PROMOTION) {
			scores.push_back({move, PieceValue[move.promotion() + KNIGHT] - PawnValue});
		}
	}

	Value best = stand_pat;
	Move best_move = NullMove;

	int alpha_raise = 0;

	Move move = NullMove;
	int end = scores.size();

	while ((move = next_move(scores, end)) != NullMove) {
		if (move.type() != PROMOTION) {
			Value see = ti.board.see_capture(move);
			if (see < 0) {
				continue; // Don't search moves that lose material
			} else {
				// QS Futility pruning
				// use see score for added safety
				if (DELTA_THRESHOLD + 4 * see + stand_pat < alpha) continue;
			}
		}

		ti.line[depth].move = move;

		ti.board.make_move(move);
		_mm_prefetch(&ttable.TT[ti.board.zobrist % ttable.TT_SIZE], _MM_HINT_T0);
		Value score = -quiesce(ti, -beta, -alpha, -side, depth + 1, pv);
		ti.board.unmake_move();

		ti.line[depth].move = NullMove;

		if (score > best) {
			if (score > alpha) {
				alpha = score;
				alpha_raise++;
			}
			best = score;
			best_move = move;
		}
		if (score >= beta) {
			ttable.store(ti.board.zobrist, score_to_tt(score, depth), raw_eval, 0, LOWER_BOUND, pv, move, depth);
			return best;
		}
	}

	ttable.store(ti.board.zobrist, score_to_tt(best, depth), raw_eval, 0, alpha_raise ? EXACT : UPPER_BOUND, pv, best_move, depth);

	return best;
}

Value __recurse(ThreadInfo &ti, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1, bool pv = false, bool cutnode = false, int ply = 0, bool root = false) {
	if (pv) ti.pvlen[ply] = 0;

	Board &board = ti.board;

	if (ply >= MAX_PLY)
		return eval(board, (BoardState *)ti.bs) * side;

	nodes[ti.id]++;

	if (stop_search) return 0;

	if (ti.is_main && !(nodes[ti.id] & 4095)) {
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		if (time > mxtime || nodes[ti.id] > mx_nodes) { // currently, the nodes will be broken but time will be accurate
			stop_search = true;
			return 0;
		}
	}

	/**
	 * Mate-distance pruning
	 * 
	 * If we are at `ply` plies away from the root and we know we already have a mate-in-N
	 * such that N <= ply, we can stop searching this branch since it's longer than our
	 * current best mate.
	 */
	alpha = std::max((int)alpha, -VALUE_MATE + ply);
	beta = std::min((int)beta, VALUE_MATE - ply + 1);
	if (alpha >= beta) {
		return alpha;
	}

	// Control on white king and black king respectively
	bool wcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK);
	bool bcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE);

	if (board.side == WHITE) {
		// If it is white to move and white controls black's king, it's mate
		if (bcontrol > 0)
			return VALUE_MATE;
	} else {
		// Likewise, the contrary also applies.
		if (wcontrol > 0)
			return VALUE_MATE;
	}

	// Threefold or 50 move rule
	if (!root && (board.threefold(ply) || board.halfmove >= 100 || board.insufficient_material())) {
		return 0;
	}

	bool in_check = false;
	if (board.side == WHITE) {
		in_check = wcontrol > 0;
	} else {
		in_check = bcontrol > 0;
	}

	if (in_check) depth++; // Check extensions

	if (depth <= 0) {
		// Reached the maximum depth, perform quiescence search
		return quiesce(ti, alpha, beta, side, ply, pv);
	}

	bool ttpv = pv;

	// Check for TTable cutoff
	TTable::TTEntry *tentry = ttable.probe(board.zobrist);
	Value tteval = 0;
	if (tentry) tteval = tt_to_score(tentry->eval, ply);
	if (!pv && tentry && tentry->depth >= depth && ti.line[ply].excl == NullMove) {
		// Check for cutoffs
		if (tentry->bound() == EXACT) {
			return tteval;
		} else if (tentry->bound() == LOWER_BOUND && tteval >= beta) {
			return tteval;
		} else if (tentry->bound() == UPPER_BOUND && tteval <= alpha) {
			return tteval;
		}
	}

	if (tentry) ttpv |= tentry->ttpv();

	Value cur_eval = 0;
	Value raw_eval = 0; // For CorrHist
	Value tt_corr_eval = 0;
	uint64_t pawn_hash = 0;
	if (!in_check) {
		pawn_hash = board.pawn_struct_hash();
		cur_eval = tentry ? tentry->s_eval : eval(board, (BoardState *)ti.bs) * side;
		raw_eval = cur_eval;
		ti.thread_hist.apply_correction(board, cur_eval);
		tt_corr_eval = cur_eval;
		if (tentry && abs(tteval) < VALUE_MATE_MAX_PLY && tentry->bound() != (tteval > cur_eval ? UPPER_BOUND : LOWER_BOUND))
			tt_corr_eval = tteval;
		else if (!tentry) ttable.store(board.zobrist, -VALUE_INFINITE, raw_eval, 0, NONE, false, NullMove, board.halfmove);
	}

	ti.line[ply].eval = in_check ? VALUE_NONE : cur_eval; // If in check, we don't have a valid eval yet

	bool improving = false;
	if (!in_check && ply >= 2 && ti.line[ply-2].eval != VALUE_NONE && cur_eval > ti.line[ply-2].eval) improving = true;

	// Reverse futility pruning
	if (!in_check && !ttpv && depth <= 8) {
		/**
		 * The idea is that if we are winning by such a large margin that we can afford to lose
		 * RFP_THRESHOLD * depth eval units per ply, we can return the current eval.
		 * 
		 * We need to make sure that we aren't in check (since we might get mated)
		 */
		int margin = (RFP_THRESHOLD - improving * RFP_IMPROVING) * depth + RFP_QUADRATIC * depth * depth;
		if (tt_corr_eval >= beta + margin)
			return (tt_corr_eval + beta) / 2;
	}

	// Null-move pruning
	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	int npawns_and_kings = _mm_popcnt_u64(board.piece_boards[PAWN] | board.piece_boards[KING]);
	if (!pv && !in_check && npieces != npawns_and_kings && tt_corr_eval >= beta && depth >= 2 && ti.line[ply].excl == NullMove) { // Avoid NMP in pawn endgames
		/**
		 * This works off the *null-move observation*.
		 * 
		 * The general idea is that a null move will almost always be worse than the best move
		 * in a given position. So, if we can play a suboptimal move (in this case the null move)
		 * and still be winning, we were probably winning in the first place.
		 * 
		 * The only issue with this approach is that it will fail in Zugzwang positions. There's
		 * really no good way of preventing this except for disabling NMP in positions where there
		 * are probably Zugzwangs (e.g. endgames).
		 */
		board.make_move(NullMove);
		// Perform a reduced-depth search
		Value r = NMP_R_VALUE + depth / 4 + std::min(3, (tt_corr_eval - beta) / 400) + improving;
		Value null_score = -__recurse(ti, depth - r, -beta, -beta + 1, -side, 0, !cutnode, ply+1);
		board.unmake_move();
		if (null_score >= beta)
			return null_score >= VALUE_MATE_MAX_PLY ? beta : null_score;
	}

	// Razoring
	if (!pv && !in_check && depth <= 8 && tt_corr_eval + RAZOR_MARGIN * depth < alpha) {
		/**
		 * If we are losing by a lot, check w/ qsearch to see if we could possibly improve.
		 * If not, we can prune the search.
		 */
		Value razor_score = quiesce(ti, alpha, beta, side, ply, 0);
		if (razor_score <= alpha)
			return razor_score;
	}

	Value best = -VALUE_INFINITE;

	MovePicker mp(board, &ti.line[ply], ply, &ti.thread_hist, tentry);

	if ((pv || cutnode) && depth > 4 && !(tentry && tentry->best_move != NullMove)) {
		depth -= 2; // Internal iterative reductions
	}

	Move best_move = NullMove;

	int alpha_raise = 0;
	TTFlag flag = UPPER_BOUND;

	pzstd::vector<Move> quiets, captures;

	Move move = NullMove;
	int i = 0;

	uint64_t prev_nodes = nodes[ti.id];

	while ((move = mp.next()) != NullMove) {
		if (move == ti.line[ply].excl)
			continue;
		
		bool capt = (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst()));
		bool promo = (move.type() == PROMOTION);
		
		int extension = 0;

		if (ti.line[ply].excl == NullMove && depth >= 8 && i == 0 && tentry && move == tentry->best_move && tentry->depth >= depth - 3 && tentry->bound() != UPPER_BOUND) {
			// Singular extension
			ti.line[ply].excl = move;
			Value singular_beta = tteval - 4 * depth;
			Value singular_score = __recurse(ti, (depth-1) / 2, singular_beta - 1, singular_beta, side, 0, cutnode, ply);
			ti.line[ply].excl = NullMove; // Reset exclusion move

			if (singular_score < singular_beta) {
				extension++;

				if (singular_score <= singular_beta - 20)
					extension++;
			} else if (tteval >= beta) {
				// Negative extensions
				extension -= 3;
			} else if (cutnode) {
				extension -= 2;
			}
		}

		ti.line[ply].move = move;

		Value hist = capt ? ti.thread_hist.get_capthist(board, move) : ti.thread_hist.get_history(board, move, ply, &ti.line[ply]);
		if (best > -VALUE_MATE_MAX_PLY) {
			if (i >= (5 + depth * depth) / (2 - improving)) {
				/**
				 * Late Move Pruning
				 * 
				 * Just skip later moves that probably aren't good
				 */
				break;
			}

			if (!in_check && !capt && !promo && depth <= 5) {
				/**
				 * History pruning
				 * 
				 * Skip moves with very bad history scores
				 * Depth condition is necessary to avoid overflow
				 */
				if (hist < -HISTORY_MARGIN * depth)
					break;
			}

			if (depth <= 3 && !promo && best > -VALUE_INFINITE) {
				/**
				 * PVS SEE Pruning
				 * 
				 * Skip searching moves with bad SEE scores
				 */
				Value see = board.see_capture(move);
				if (see < (-50 - 50 * capt) * depth)
					continue;
			}
		}

		ti.line[ply].cont_hist = &ti.thread_hist.cont_hist[board.side][board.mailbox[move.src()] & 7][move.dst()];

		board.make_move(move);

		_mm_prefetch(&ttable.TT[board.zobrist % ttable.TT_SIZE], _MM_HINT_T0);

		Value newdepth = depth - 1 + extension;

		/**
		 * PV Search (principal variation)
		 * 
		 * Assuming our move ordering is good, there probably won't be any moves past
		 * the first one that are better than that first move. So, we run a reduced-depth
		 * null-window search on later moves (a much shorter search) to ensure that they
		 * are bad moves. 
		 * 
		 * However, if the move turns out to be better than expected, we run a full-window
		 * full-depth re-search. This, however, doesn't happen often enough to slow down
		 * the search.
		 */
		Value score;
		if (depth >= 2 && i >= 1 + 2 * pv) {
			Value r = reduction[i][depth];

			// Base reduction offset
			r -= 1024;

			r -= 1024 * pv;
			r += 1024 * (!pv && cutnode);
			if (move == ti.line[ply].killer[0] || move == ti.line[ply].killer[1])
				r -= 1024;
			r -= 1024 * ttpv;
			r -= hist / 16 * !capt;

			Value searched_depth = newdepth - r / 1024;

			score = -__recurse(ti, searched_depth, -alpha - 1, -alpha, -side, 0, true, ply+1);
			if (score > alpha && searched_depth < newdepth) {
				score = -__recurse(ti, newdepth, -alpha - 1, -alpha, -side, 0, !cutnode, ply+1);
			}
		} else if (!pv || i > 0) {
			score = -__recurse(ti, newdepth, -alpha - 1, -alpha, -side, 0, !cutnode, ply+1);
		}
		if (pv && (i == 0 || score > alpha)) {
			if (tentry && move == tentry->best_move && tentry->depth > 1)
				newdepth = std::max((int)newdepth, 1); // Make sure we don't enter QS if we have an available TT move
			score = -__recurse(ti, newdepth, -beta, -alpha, -side, 1, false, ply+1);
		}

		board.unmake_move();

		ti.line[ply].cont_hist = nullptr;

		if (root) {
			nodecnt[move.src()][move.dst()] += nodes[ti.id] - prev_nodes;
			prev_nodes = nodes[ti.id];
		}

		if (stop_search)
			break;

		if (score > best) {
			if (score > alpha) {
				best_move = move;
				alpha = score;
				alpha_raise++;
				flag = EXACT;
				if (score < beta) {
					ti.pvtable[ply][0] = move;
					ti.pvlen[ply] = ti.pvlen[ply+1]+1;
					for (int j = 0; j < ti.pvlen[ply+1]; j++) {
						ti.pvtable[ply][j+1] = ti.pvtable[ply+1][j];
					}
				}
			}
			best = score;
		}

		if (score >= beta) {
			flag = LOWER_BOUND;
			if (abs(score) < VALUE_MATE_MAX_PLY && abs(alpha) < VALUE_MATE_MAX_PLY) {
				// note that best and score are functionally equivalent here; best is just what's returned + stored to TT
				best = (score * depth + beta) / (depth + 1); // wtf?????
			}
			if (ti.line[ply].killer[0] != move) {
				ti.line[ply].killer[1] = ti.line[ply].killer[0];
				ti.line[ply].killer[0] = move; // Update killer moves
			}
			const Value bonus = std::min(1896, 4 * depth * depth + 120 * depth - 120); // saturate updates at depth 12
			if (!capt) { // Not a capture
				ti.thread_hist.update_history(board, move, ply, &ti.line[ply], bonus);
				for (auto &qmove : quiets) {
					ti.thread_hist.update_history(board, qmove, ply, &ti.line[ply], -bonus); // Penalize quiet moves
				}
			} else { // Capture
				ti.thread_hist.update_capthist(PieceType(board.mailbox[move.src()] & 7), PieceType(board.mailbox[move.dst()] & 7), move.dst(), bonus);
			}
			for (auto &cmove : captures) {
				ti.thread_hist.update_capthist(PieceType(board.mailbox[cmove.src()] & 7), PieceType(board.mailbox[cmove.dst()] & 7), cmove.dst(), -bonus);
			}
			break;
		}

		if (!capt && !promo) quiets.push_back(move);
		else if (capt) captures.push_back(move);
		i++;
	}

	// Stalemate detection
	if (best == -VALUE_MATE) {
		// If our engine thinks we are mated but we are not in check, we are stalemated
		if (ti.line[ply].excl != NullMove) return alpha;
		else if (in_check) return -VALUE_MATE + ply;
		else return 0;
	}

	bool best_iscapture = (board.piece_boards[OPPOCC(board.side)] & square_bits(best_move.dst()));
	bool best_ispromo = (best_move.type() == PROMOTION);
	if (!in_check && !(best_move != NullMove && (best_iscapture || best_ispromo))
		&& !(flag == UPPER_BOUND && best >= raw_eval) && !(flag == LOWER_BOUND && best <= raw_eval)) {
		// Best move is a quiet move, update CorrHist
		int bonus = (best - raw_eval) * depth / 8;
		ti.thread_hist.update_corrhist(board, bonus);
	}

	if (ti.line[ply].excl == NullMove) {
		Move tt_move = best_move != NullMove ? best_move : tentry ? tentry->best_move : NullMove;
		ttable.store(board.zobrist, score_to_tt(best, ply), raw_eval, depth, flag, ttpv, tt_move, board.halfmove);
	}

	return best;
}

void iterativedeepening(ThreadInfo &ti, int depth) {
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			ti.thread_hist.history[0][i][j] /= 2;
			ti.thread_hist.history[1][i][j] /= 2;
		}
	}

	Board &board = ti.board;

	Value static_eval = eval(board, (BoardState *)ti.bs) * (board.side ? -1 : 1);

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	for (int d = 1; d <= depth; d++) {
		Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		Value window_sz = ASPIRATION_WINDOW;

		if (eval != -VALUE_INFINITE) {
			/**
			 * Aspiration windows work by searching a small window around the expected value
			 * of the position. By having a smaller window, our search runs faster. 
			 * 
			 * If we fail either high or low out of this window, we gradually expand the
			 * window size, eventually getting to a full-width search.
			 */
			alpha = eval - window_sz;
			beta = eval + window_sz;
		}

		auto result = __recurse(ti, d, alpha, beta, board.side ? -1 : 1, 1, false, 0, true);

		// Gradually expand the window if we fail high or low
		while ((result >= beta || result <= alpha) && window_sz < VALUE_INFINITE / 4) {
			if (result >= beta) {
				// Fail high - expand upper bound
				beta = eval + window_sz * 2;
				if (beta >= VALUE_INFINITE / 4) beta = VALUE_INFINITE;
			}
			if (result <= alpha) {
				// Fail low - expand lower bound  
				alpha = eval - window_sz * 2;
				if (alpha <= -VALUE_INFINITE / 4) alpha = -VALUE_INFINITE;
			}
			window_sz *= 2;
			result = __recurse(ti, d, alpha, beta, board.side ? -1 : 1, 1, false, 0, true);
			if (stop_search) break;
		}
		if (stop_search) break;
		eval = result;
		best_move = ti.pvtable[0][0];
		
		if (ti.is_main) {
			// We must calculate best move nodes and total nodes at around the same time
			// so that node counts don't change in between due to race conditions
			// This is a really crude way of doing it (todo change later)
			uint64_t bm_nodes = nodecnt[best_move.src()][best_move.dst()];
			uint64_t tot_nodes = 0;
			for (int t = 0; t < num_threads; t++) {
				tot_nodes += nodes[t]; // ig this is dangerous but whatever
			}

			// UCI output from main thread only
			auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
			std::cout << "info depth " << d << " score " << score_to_uci(eval) << " time " << time_elapsed << " nodes " << tot_nodes << " nps "
					  << (time_elapsed ? (tot_nodes * 1000 / time_elapsed) : tot_nodes) << " hashfull " << (int)(get_ttable_sz() * 1000) << " pv";
			for (int ply = 0; ply < ti.pvlen[0]; ply++) {
				std::cout << " " << ti.pvtable[0][ply].to_string();
			}
			std::cout << std::endl;

			// only do time management on main thread
			bool best_iscapt = board.is_capture(best_move);
			bool best_ispromo = (best_move.type() == PROMOTION);
			bool in_check = false;
			if (board.side == WHITE) {
				in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK) > 0;
			} else {
				in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE) > 0;
			}

			double soft = 0.5;
			if (depth >= 6 && !best_iscapt && !best_ispromo && !in_check) {
				// adjust soft limit based on complexity
				Value complexity = abs(eval - static_eval);
				double factor = std::clamp(complexity / 200.0, 0.0, 1.0);
				// higher complexity = spend more time, lower complexity = spend less time
				soft = 0.3 + 0.4 * factor;
			}

			double node_adjustment = 1.5 - (bm_nodes / (double)tot_nodes);
			soft *= node_adjustment;
			if (time_elapsed > mxtime * soft) {
				// We probably won't be able to complete the next ID loop
				stop_search = true;
				break;
			}
		}

		ti.maxdepth = d;
	}

	ti.eval = eval;
	stop_search = true;

	if (ti.is_main) {
		std::cout << "bestmove " << best_move.to_string() << std::endl;
	}
}

std::pair<Move, Value> search(Board &board, ThreadInfo *threads, int64_t time, int depth, int64_t maxnodes, int quiet) {
	for (int i = 0; i < 64; i++) for (int j = 0; j < 64; j++) nodecnt[i][j] = 0;

	mxtime = time;
	mx_nodes = maxnodes;
	start = std::chrono::steady_clock::now();
	stop_search = false;

	Move best_move = NullMove;
	Value eval = 0;

	std::vector<std::thread> thread_handles;

	for (int t = 0; t < num_threads; t++) {
		ThreadInfo &ti = threads[t];
		ti.board = board;
		nodes[t] = 0;
		ti.id = t;
		ti.is_main = (t == 0);
		// don't clear search vars here; keep history
		thread_handles.emplace_back(iterativedeepening, std::ref(ti), depth);
	}

	for (int t = 0; t < num_threads; t++) {
		thread_handles[t].join();
	}

	ThreadInfo &best_thread = threads[0];

	return {best_thread.pvtable[0][0], best_thread.eval};
}

void clear_search_vars(ThreadInfo &ti) {
	ti.board.reset_startpos();
	memset(&ti.thread_hist, 0, sizeof(History));
	for (int i = 0; i < MAX_PLY; i++) {
		ti.line[i] = SSEntry();
	}
}
