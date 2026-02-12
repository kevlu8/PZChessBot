#include "search.hpp"
#include <utility>

#define MOVENUM(x) ((((#x)[1] - '1') << 12) | (((#x)[0] - 'a') << 8) | (((#x)[3] - '1') << 4) | ((#x)[2] - 'a'))

uint64_t mx_nodes = 1e18; // Maximum nodes to search
bool stop_search = true;
std::chrono::steady_clock::time_point start;
uint64_t mxtime = 1e18; // Maximum time to search in milliseconds
int minimal = 0;
std::stringstream last_line;

uint16_t num_threads = 1;

std::atomic<uint64_t> nodecnt[64][64] = {{}};
std::atomic<uint64_t> nodes[MAX_THREADS] = {};

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
			else reduction[i][d] = (0.76 + log(i) * log(d) / 2.32) * 1024;
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

/**
 * Select the next move from the list of scored moves
 * 
 * This function uses a basic partial selection-sort-like algorithm to select the move with
 * the highest score. It then swaps the move to the back of the list, reducing the effective
 * time of the list by 1/2.
 * 
 * This approach is chosen because a lot of the time, we only need the top few moves from the
 * list to produce a cutoff.
 */
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

/**
 * Convert a score to a TTable-storable score
 * 
 * Because our search function returns mate scores as an absolute mate-in-ply,
 * we need to convert them to a relative mate score for storage in the TTable.
 * 
 * Note that returning relative mate scores is fundamentally incompatible with the
 * alpha-beta search algorithm, hence why this conversion is necessary.
 */
Value score_to_tt(Value score, int ply) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return score + ply;
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return score - ply;
	} else {
		return score;
	}
}

/**
 * Convert a TTable-stored score to a search-returnable score
 */
Value tt_to_score(Value score, int ply) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return score - ply;
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return score + ply;
	} else {
		return score;
	}
}

/**
 * Get the usage of the transposition table
 * 
 * Used for UCI output. This function samples the first 1024 entries of the TTable
 * then counts how many are occupied.
 */
double get_ttable_sz() {
	int cnt = 0;
	for (int i = 0; i < 1024; i++) {
		if (i >= ttable.TT_SIZE) break;
		if (ttable.TT[i].entries[0].valid() && ttable.TT[i].entries[0].age() == ttable.age) cnt++;
		if (ttable.TT[i].entries[1].valid() && ttable.TT[i].entries[1].age() == ttable.age) cnt++;
		if (ttable.TT[i].entries[2].valid() && ttable.TT[i].entries[2].age() == ttable.age) cnt++;
	}
	return cnt / (3.0 * 1024);
}

/**
 * Convert a score to UCI format
 * 
 * UCI format requires mate scores to be represented as "mate N" where N is full-moves to
 * mate. Thus, we need to convert our internal representation to this format.
 */
std::string score_to_uci(Value score) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((VALUE_MATE - score + 1) / 2);
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((-VALUE_MATE - score) / 2);
	} else {
		return "cp " + std::to_string(int(score / NNUE_PAWN_VALUE));
	}
}

/**
 * Checks if a score is valid
 */
bool is_valid_score(Value score) {
	return score > -VALUE_INFINITE && score < VALUE_INFINITE;
}

Value negamax(ThreadInfo &ti, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1, bool pv = false, bool cutnode = false, int ply = 0, bool root = false) {
	Board &board = ti.board;

	if (ply >= MAX_PLY)
		return eval(board, (BoardState *)ti.bs) * side;

	if (pv) {
		ti.pvlen[ply] = 0;
		ti.seldepth = std::max(ti.seldepth, ply);
	}

	nodes[ti.id]++;

	if (stop_search) return 0;

	if (ti.is_main && !(nodes[ti.id] & 4095)) {
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		if (time > mxtime) { // currently, the nodes will be broken but time will be accurate
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
	// alpha = std::max((int)alpha, -VALUE_MATE + ply);
	// beta = std::min((int)beta, VALUE_MATE - ply + 1);
	// if (alpha >= beta) {
	// 	return alpha;
	// }

	// Threefold or 50 move rule
	if (!root && (board.threefold(ply) || board.halfmove >= 100)) {
		return 0;
	}

	bool ttpv = pv;
	bool forced_capture = false;
	pzstd::vector<Move> tmp_moves;
	board.legal_moves(tmp_moves);
	for (auto &m : tmp_moves) {
		if (board.is_capture(m)) {
			forced_capture = true;
			break;
		}
	}

	if (tmp_moves.size() == 0) // We win
		return VALUE_MATE;

	if (depth <= 0 && !forced_capture) {
		// Reached the maximum depth, perform quiescence search
		// return quiesce(ti, alpha, beta, side, ply, pv);
		return eval(board, (BoardState *)ti.bs) * side;
	} else if (depth <= 0) depth = 1; // do not drop to eval if captures still exist

	/**
	 * TTable Cutoffs
	 * 
	 * If we have already searched this position at a sufficient depth and with the right
	 * bounds, we can use the stored evaluation to directly cut off our search.
	 * 
	 * Note that we cannot do this in singular search (`ti.line[ply].excl != NullMove`)
	 * because the singular search excludes a move that may be the best move in the position.
	 */
	auto tentry = ttable.probe(board.zobrist);
	Value tteval = -VALUE_INFINITE;
	if (tentry && is_valid_score(tentry->eval)) tteval = tt_to_score(tentry->eval, ply);
	if (!pv && tentry && is_valid_score(tteval) && tentry->depth >= depth && ti.line[ply].excl == NullMove) {
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

	// Evaluate and correct evaluation
	Value cur_eval = 0;
	Value raw_eval = 0;
	Value tt_corr_eval = 0;
	if (!forced_capture) {
		cur_eval = tentry ? tentry->s_eval : eval(board, (BoardState *)ti.bs) * side;
		raw_eval = cur_eval;
		ti.thread_hist.apply_correction(board, &ti.line[ply], ply, cur_eval);
		tt_corr_eval = cur_eval;
		if (tentry && is_valid_score(tteval) && abs(tteval) < VALUE_MATE_MAX_PLY && tentry->bound() != (tteval > cur_eval ? UPPER_BOUND : LOWER_BOUND))
			tt_corr_eval = tteval;
		else if (!tentry) ttable.store(board.zobrist, -VALUE_INFINITE, raw_eval, 0, NONE, false, NullMove);
	}

	ti.line[ply].eval = forced_capture ? VALUE_NONE : cur_eval; // If in check, we don't have a valid eval yet

	/**
	 * Improving flag
	 * 
	 * If our position has gotten better since the last time it was our turn, we say that we are *improving*.
	 * We can use this information to modify some of our pruning techniques.
	 */
	bool improving = false;
	if (!forced_capture && ply >= 2 && ti.line[ply-2].eval != VALUE_NONE && cur_eval > ti.line[ply-2].eval) improving = true;

	// Reverse futility pruning
	if (!forced_capture && !ttpv && depth <= 8) {
		/**
		 * The idea is that if we are winning by such a large margin that we can afford to lose
		 * RFP_THRESHOLD * depth eval units per ply, we can return the current eval.
		 * 
		 * We need to make sure that we aren't in check (since we might get mated)
		 */
		int margin = (80 - improving * 20) * depth;
		if (tt_corr_eval >= beta + margin)
			return ((int)tt_corr_eval + beta) / 2;
	}

	// // Razoring
	// if (!pv && !forced_capture && depth <= 8 && tt_corr_eval + RAZOR_MARGIN * depth < alpha) {
	// 	/**
	// 	 * If we are losing by a lot, check w/ qsearch to see if we could possibly improve.
	// 	 * If not, we can prune the search.
	// 	 */
	// 	Value razor_score = quiesce(ti, alpha, beta, side, ply, 0);
	// 	if (razor_score <= alpha)
	// 		return razor_score;
	// }

	// // Probcut
	// if (!pv && !forced_capture && depth >= 7 && abs(beta) < VALUE_MATE_MAX_PLY && !(tentry && is_valid_score(tteval) && tteval < beta + PROBCUT_MARGIN)) {
	// 	/**
	// 	 * ProbCut
	// 	 * 
	// 	 * Before running search, we generate moves that we think are good and
	// 	 * search them with a reduced depth and a higher beta. If one of them fails
	// 	 * high, we assume that the node will fail high at full depth as well.
	// 	 * 
	// 	 * Note that if we have strong evidence that ProbCut will fail (e.g. from the TT),
	// 	 * we skip ProbCut to save time.
	// 	 * 
	// 	 * In the body of the ProbCut loop, we first run a QSearch to figure out whether
	// 	 * or not the move could cut. If the QSearch doesn't fail high, we skip the move
	// 	 * in order to save effort.
	// 	 */
	// 	MovePicker pcpicker(board, &ti.thread_hist, tentry);
	// 	Move pc_move = NullMove;
	// 	int pc_depth = depth - 5;
	// 	Value pc_beta = beta + PROBCUT_MARGIN;
	// 	while ((pc_move = pcpicker.next()) != NullMove) {
	// 		if (pc_move == ti.line[ply].excl)
	// 			continue;

	// 		ti.line[ply].move = pc_move;
	// 		ti.line[ply].captured = (PieceType)(board.mailbox[pc_move.dst()] & 7);
	// 		ti.line[ply].piece = (PieceType)(board.mailbox[pc_move.src()] & 7);
	// 		ti.line[ply].cont_hist = &ti.thread_hist.cont_hist[board.side][board.mailbox[pc_move.src()] & 7][pc_move.dst()];
	// 		ti.line[ply].corr_hist = &ti.thread_hist.corrhist_cont[board.side][board.mailbox[pc_move.src()] & 7][pc_move.dst()];

	// 		board.make_move(pc_move);
	// 		_mm_prefetch(&ttable.TT[board.zobrist & (ttable.TT_SIZE - 1)], _MM_HINT_T0);
	// 		Value score = -quiesce(ti, -pc_beta, -pc_beta + 1, -side, ply + 1);

	// 		if (score >= pc_beta)
	// 			score = -negamax(ti, pc_depth, -pc_beta, -pc_beta + 1, -side, 0, !cutnode, ply + 1);

	// 		board.unmake_move();

	// 		ti.line[ply].move = NullMove;
	// 		ti.line[ply].captured = NO_PIECETYPE;
	// 		ti.line[ply].piece = NO_PIECETYPE;
	// 		ti.line[ply].cont_hist = nullptr;
	// 		ti.line[ply].corr_hist = nullptr;

	// 		if (score >= pc_beta) {
	// 			ttable.store(board.zobrist, score_to_tt(score, ply), raw_eval, pc_depth + 1, LOWER_BOUND, false, pc_move);
	// 			return score;
	// 		}
	// 	}
	// }

	Value best = -VALUE_INFINITE;

	MovePicker mp(board, &ti.line[ply], ply, &ti.thread_hist, tentry);

	// Internal iterative reductions
	if ((pv || cutnode) && depth > 4 && !(tentry && tentry->best_move != NullMove)) {
		/**
		 * In positions where we are in a node where we'd expect a TT move (i.e. PV/cutnode) and
		 * we don't have one, it's likely that the position is not good enough to warrant a TT move.
		 * Thus, we can reduce the depth of the search slightly.
		 */
		depth -= 2;
	}

	Move best_move = NullMove;

	int alpha_raise = 0;
	TTFlag flag = UPPER_BOUND;

	pzstd::vector<Move> quiets, captures;

	Move move = NullMove;
	int i = 0;

	uint64_t prev_nodes = nodes[ti.id];

	ti.line[ply+1].cutoffcnt = 0;

	while ((move = mp.next()) != NullMove) {
		if (move == ti.line[ply].excl)
			continue;
		
		bool capt = board.is_capture(move);
		bool promo = (move.type() == PROMOTION);

		if (forced_capture && !capt) continue;

		int extension = 0;

		if (ti.line[ply].excl == NullMove && depth >= 8 && tentry && is_valid_score(tteval) && move == tentry->best_move && tentry->depth >= depth - 3 && tentry->bound() != UPPER_BOUND) {
			/**
			 * Singular extensions
			 * 
			 * If we are in a node where one move is significantly better than all other moves, we can
			 * extend that move (i.e. search it deeper) because it is probably very important.
			 * 
			 * We do this by excluding the move we believe is superior, then running a reduced-depth search
			 * to verify if the position indeed is far worse without that move. If it is, we extend the move.
			 * 
			 * We can also extend more if the position without the move is *very* bad.
			 */
			ti.line[ply].excl = move;
			Value singular_beta = tteval - 2 * depth;
			Value singular_score = negamax(ti, (depth-1) / 2, singular_beta - 1, singular_beta, side, 0, cutnode, ply);
			ti.line[ply].excl = NullMove; // Reset exclusion move

			if (singular_score < singular_beta) {
				extension++;

				if (singular_score <= singular_beta - 23)
					extension++;
			} else if (singular_score >= beta)
				/**
				 * Multicut
				 * 
				 * If the search even without the expected best move still fails
				 * high, we can almost certainly conclude that there are multiple
				 * moves that are very good and that it will cut off.
				 */
				return singular_score;
			else if (tteval >= beta) {
				/**
				 * Negative extensions
				 * 
				 * singular_beta <= singular_score < beta && tteval >= beta
				 * 
				 * The TT suggested the evaluation is actually good enough to cause a beta cutoff,
				 * but even after banning the move the position is still good. We can deprioritize
				 * the TT move slightly, in favor of hopefully finding a better move.
				 */
				extension -= 3;
			} else if (cutnode) {
				/**
				 * singular_beta <= singular_score < beta && tteval < beta && cutnode
				 * 
				 * The TT suggests a non-cutoff, but we expect a cutoff to occur. We also know that
				 * the move isn't singular, so we can reduce the depth slightly in favor of other moves.
				 */
				extension -= 2;
			}
		}

		int hist = capt ? ti.thread_hist.get_capthist(board, move) : ti.thread_hist.get_history(board, move, ply, &ti.line[ply]);
		if (best > -VALUE_MATE_MAX_PLY) {
			if (!root && i >= (5 + depth * depth) / (2 - improving)) {
				/**
				 * Late Move Pruning
				 * 
				 * Moves that are ordered far near the back probably aren't very good, so we can
				 * directly skip them.
				 */
				break;
			}

			Value futility = cur_eval + 300 + 100 * depth + hist / 32;
			if (!forced_capture && !capt && !promo && depth <= 5 && futility <= alpha) {
				/**
				 * Futility pruning
				 * 
				 * If we are losing by a lot, and this move is unlikely to improve our position,
				 * skip searching it along with all quiet moves.
				 */
				mp.skip_quiets();
				continue;
			}

			if (!forced_capture && !capt && !promo && depth <= 5) {
				/**
				 * History pruning
				 * 
				 * Skip moves with very bad history scores
				 * Depth condition is necessary to avoid overflow
				 */
				if (hist < -HISTORY_MARGIN * depth)
					break;
			}
		}

		ti.line[ply].move = move;
		ti.line[ply].captured = (PieceType)(board.mailbox[move.dst()] & 7);
		ti.line[ply].piece = (PieceType)(board.mailbox[move.src()] & 7);
		ti.line[ply].cont_hist = &ti.thread_hist.cont_hist[board.side][board.mailbox[move.src()] & 7][move.dst()];
		ti.line[ply].corr_hist = &ti.thread_hist.corrhist_cont[board.side][board.mailbox[move.src()] & 7][move.dst()];

		board.make_move(move);

		_mm_prefetch(&ttable.TT[board.zobrist & (ttable.TT_SIZE - 1)], _MM_HINT_T0);

		int newdepth = depth - 1 + extension;

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
			// Case 1: Late moves in nodes

			int r = reduction[i][depth];

			// Base reduction
			r -= 372;

			r -= 1062 * pv; // Reduce less in PV nodes
			r += 1303 * cutnode; // Reduce more in cutnodes
			r += 918 * (ti.line[ply+1].cutoffcnt > 3);
			r -= 975 * ttpv;

			if (move == ti.line[ply].killer) {
				r -= 932;
			}

			if (!capt && !promo)
				r -= hist / 8;

			if (capt || promo)
				r = std::min(r, 1024); // Do not reduce captures or promotions

			int searched_depth = std::clamp(newdepth - r / 1024, 1, newdepth + 1);

			score = -negamax(ti, searched_depth, -alpha - 1, -alpha, -side, 0, true, ply+1);
			if (score > alpha && searched_depth < newdepth) {
				// LMR search failed, re-search full depth
				bool do_deeper = score > beta + 100;
				newdepth += do_deeper;

				score = -negamax(ti, newdepth, -alpha - 1, -alpha, -side, 0, !cutnode, ply+1);
			}
		} else if (!pv || i > 0) {
			// Case 2: Early moves in nodes
			score = -negamax(ti, newdepth, -alpha - 1, -alpha, -side, 0, !cutnode, ply+1);
		}
		if (pv && (i == 0 || score > alpha)) {
			// Case 3: First PV node move or re-search
			if (tentry && move == tentry->best_move && tentry->depth > 1)
				newdepth = std::max((int)newdepth, 1); // Make sure we don't enter QS if we have an available TT move
			score = -negamax(ti, newdepth, -beta, -alpha, -side, 1, false, ply+1);
		}

		board.unmake_move();

		ti.line[ply].move = NullMove;
		ti.line[ply].captured = NO_PIECETYPE;
		ti.line[ply].piece = NO_PIECETYPE;
		ti.line[ply].cont_hist = nullptr;
		ti.line[ply].corr_hist = nullptr;

		if (root) {
			nodecnt[move.src()][move.dst()] += nodes[ti.id] - prev_nodes;
			prev_nodes = nodes[ti.id];
		}

		if (stop_search)
			break;

		if (score <= alpha) {
			if (capt)
				captures.push_back(move);
			else
				quiets.push_back(move);
		}

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
			ti.line[ply].cutoffcnt++;
			flag = LOWER_BOUND;
			if (abs(score) < VALUE_MATE_MAX_PLY && abs(alpha) < VALUE_MATE_MAX_PLY) {
				// note that best and score are functionally equivalent here; best is just what's returned + stored to TT
				best = (score * depth + beta) / (depth + 1); // wtf?????
			}
			if (ti.line[ply].killer != move) {
				ti.line[ply].killer = move; // Update killer move
			}
			const Value bonus = std::min(1896, 3 * depth * depth + 109 * depth - 142);
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

		i++;
	}

	// Stalemate detection
	// Second condition necessary for positions where one side is boxed in and can't move, but we can't mate
	// e.g. 8/8/8/8/ppp1p3/krp1p2K/nbp1p3/nqrbN3 b - - 1 6
	if (best == -VALUE_MATE) {
		// If our engine thinks we are mated but we are not in check, we are stalemated
		if (ti.line[ply].excl != NullMove) return alpha;
		else return -VALUE_MATE + ply;
	}
	if (best == -VALUE_INFINITE)
		return VALUE_MATE; // no legal moves

	bool best_iscapture = (board.piece_boards[OPPOCC(board.side)] & square_bits(best_move.dst()));
	bool best_ispromo = (best_move.type() == PROMOTION);
	if (ti.line[ply].excl == NullMove && !forced_capture && !(best_move != NullMove && (best_iscapture || best_ispromo))
		&& !(flag == UPPER_BOUND && best >= cur_eval) && !(flag == LOWER_BOUND && best <= cur_eval)) {
		// Best move is a quiet move, update CorrHist
		int bonus = (best - cur_eval) * depth / 8;
		ti.thread_hist.update_corrhist(board, &ti.line[ply], ply, bonus);
	}

	if (ti.line[ply].excl == NullMove) {
		Move tt_move = best_move != NullMove ? best_move : tentry ? tentry->best_move : NullMove;
		ttable.store(board.zobrist, score_to_tt(best, ply), raw_eval, depth, flag, ttpv, tt_move);
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

	depth = std::min(depth, MAX_PLY - 1);

	Board &board = ti.board;

	Value static_eval = eval(board, (BoardState *)ti.bs) * (board.side ? -1 : 1);

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	for (int d = 1; d <= depth; d++) {
		int alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		int window_sz = ASPIRATION_WINDOW;

		if (eval != -VALUE_INFINITE && d >= 4) {
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

		auto result = negamax(ti, d, alpha, beta, board.side ? -1 : 1, 1, false, 0, true);
		int asp_depth = d;

		// Gradually expand the window if we fail high or low
		while ((result >= beta || result <= alpha)) {
			if (result >= beta) {
				// Fail high - expand upper bound
				alpha = (alpha + beta) / 2;
				beta = eval + window_sz * 2;
				asp_depth = std::max(asp_depth - 1, d - 3);
			}
			if (result <= alpha) {
				// Fail low - expand lower bound  
				alpha = eval - window_sz * 2;
			}
			if (window_sz >= VALUE_INFINITE / 4) { // give up, just use full window
				alpha = -VALUE_INFINITE;
				beta = VALUE_INFINITE;
			}
			alpha = std::clamp(alpha, -VALUE_INFINITE, (int)VALUE_INFINITE);
			beta = std::clamp(beta, -VALUE_INFINITE, (int)VALUE_INFINITE);
			window_sz *= 2;
			result = negamax(ti, asp_depth, alpha, beta, board.side ? -1 : 1, 1, false, 0, true);
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
			last_line.str("");
			last_line << "info depth " << d << " seldepth " << ti.seldepth << " score " << score_to_uci(eval) << " time " << time_elapsed << " nodes " << tot_nodes << " nps "
					  << (time_elapsed ? (tot_nodes * 1000 / time_elapsed) : tot_nodes) << " hashfull " << (int)(get_ttable_sz() * 1000) << " pv";
			for (int ply = 0; ply < ti.pvlen[0]; ply++) {
				last_line << " " << ti.pvtable[0][ply].to_string();
			}
			if (!minimal) std::cout << last_line.str() << std::endl;

			// only do time management on main thread
			bool best_iscapt = board.is_capture(best_move);
			bool best_ispromo = (best_move.type() == PROMOTION);
			bool in_check = false;

			double soft = 0.573154;
			if (depth >= 6 && !best_iscapt && !best_ispromo && !in_check) {
				// adjust soft limit based on complexity
				Value complexity = abs(eval - static_eval);
				double factor = std::clamp(complexity / 200.0, 0.0, 1.0);
				// higher complexity = spend more time, lower complexity = spend less time
				soft = 0.33232 + 0.45762 * factor;
			}

			double node_adjustment = 1.414479 - 0.931647 * (bm_nodes / (double)tot_nodes);
			soft *= node_adjustment;
			if (time_elapsed > mxtime * soft || nodes[ti.id] > mx_nodes) {
				// We probably won't be able to complete the next ID loop
				stop_search = true;
				break;
			}
		}

		ti.maxdepth = d;
	}

	ti.eval = eval;

	if (ti.is_main) {
		stop_search = true;
		if (minimal == 1) std::cout << last_line.str() << std::endl;
		if (minimal != 2) std::cout << "bestmove " << best_move.to_string() << std::endl;
	}
}

std::pair<Move, Value> search(Board &board, ThreadInfo *threads, int64_t time, int depth, int64_t maxnodes, int quiet) {
	for (int i = 0; i < 64; i++) for (int j = 0; j < 64; j++) nodecnt[i][j] = 0;

	mxtime = time;
	mx_nodes = maxnodes;
	start = std::chrono::steady_clock::now();
	stop_search = false;
	minimal = quiet;

	Move best_move = NullMove;
	Value eval = 0;

	std::vector<std::thread> thread_handles;

	for (int t = 0; t < num_threads; t++) {
		ThreadInfo &ti = threads[t];
		ti.board = board;
		ti.seldepth = 0;
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

	ttable.inc_gen();

	return {best_thread.pvtable[0][0], best_thread.eval};
}

void clear_search_vars(ThreadInfo &ti) {
	ti.board.reset_startpos();
	memset(&ti.thread_hist, 0, sizeof(History));
	for (int i = 0; i < MAX_PLY; i++) {
		ti.line[i] = SSEntry();
	}
}
