#include "search.hpp"

#define MOVENUM(x) ((((#x)[1] - '1') << 12) | (((#x)[0] - 'a') << 8) | (((#x)[3] - '1') << 4) | ((#x)[2] - 'a'))

uint64_t nodes = 0; // Node count
int seldepth = 0; // Maximum searched depth, including quiescence search
uint64_t mx_nodes = 1e18; // Maximum nodes to search
uint64_t mxtime = 1000; // Maximum time to search in milliseconds
bool early_exit = false, exit_allowed = false; // Whether or not to exit the search, and if we are allowed to exit (so we don't exit on the depth 1)
clock_t start = 0;

uint64_t perft(Board &board, int depth) {
	// If white's turn is beginning and black is in check
	if (board.side == WHITE && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])).first)
		return 0;
	// If black's turn is beginning and white is in check
	else if (board.side == BLACK && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])).second)
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

__attribute__((constructor)) void init_lmr(int i, int d) {
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

/**
 * Killer moves are a heuristic for move ordering that helps sort consistently good moves.
 * What is a killer move? It's a move that has been successful in the past, i.e. it has
 * caused a beta cutoff in the past. We store two killer moves per depth, and we add
 * bonuses to the killer moves in the move ordering function.
 */
Move killer[2][MAX_PLY];

/**
 * The history heuristic is a move ordering heuristic that helps sort quiet moves.
 * It works by storing its effectiveness in the past through beta cutoffs.
 * We store a history table for each side indexed by [src][dst].
 */
Value history[2][64][64];

/**
 * Capture history is a heuristic similar to the history heuristic, but it's used for
 * captures. It basically replaces LVA.
 */
Value capthist[6][6][64]; // [piece][captured piece][dst]

/**
 * Static Evaluation Correction History (CorrHist) is a heuristic that "corrects"
 * the static evaluation towards the actual evaluation of the position, based on
 * a variety of factors (e.g. pawn structure, minor pieces, etc)
 */
Value corrhist_ps[2][CORRHIST_SZ]; // [side][pawn hash]
Value corrhist_mat[2][CORRHIST_SZ]; // [side][material hash]

/**
 * The counter-move heuristic is a move ordering heuristic that helps sort moves that
 * have refuted other moves in the past. It works by storing the move upon a beta cutoff.
 */
Move cmh[2][64][64];

SSEntry line[MAX_PLY]; // Currently searched line

Move pvtable[MAX_PLY][MAX_PLY];
int pvlen[MAX_PLY];

/**
 * Use the history gravity formula to update our history values
 */
void update_history(bool side, Square from, Square to, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	history[side][from][to] += cbonus - history[side][from][to] * abs(bonus) / MAX_HISTORY;
}

void update_capthist(PieceType piece, PieceType captured, Square dst, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	capthist[piece][captured][dst] += cbonus - capthist[piece][captured][dst] * abs(bonus) / MAX_HISTORY;
}

// Moving exponential average for corrhist
void update_corrhist(bool side, uint64_t pshash, uint64_t mathash, Value diff, int depth) {
	const Value sdiff = diff * CORRHIST_GRAIN;
	const Value weight = std::min(depth*depth, 128);
	Value &pscorr = corrhist_ps[side][pshash % CORRHIST_SZ];
	pscorr = std::clamp((pscorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
	Value &matcorr = corrhist_mat[side][mathash % CORRHIST_SZ];
	matcorr = std::clamp((matcorr * (CORRHIST_WEIGHT - weight) + sdiff * weight) / CORRHIST_WEIGHT, -MAX_HISTORY, (int)MAX_HISTORY);
}

void apply_correction(bool side, uint64_t pshash, uint64_t mathash, Value &eval) {
	if (abs(eval) >= VALUE_MATE_MAX_PLY)
		return; // Don't apply correction if we are already at a mate score
	const Value pscorr = corrhist_ps[side][pshash % CORRHIST_SZ];
	const Value matcorr = corrhist_mat[side][mathash % CORRHIST_SZ];
	const Value corr = (pscorr + matcorr) / 2;
	eval = std::clamp(eval + corr / CORRHIST_GRAIN, -VALUE_MATE_MAX_PLY + 1, VALUE_MATE_MAX_PLY - 1);
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
Value quiesce(Board &board, Value alpha, Value beta, int side, int depth) {
	nodes++;

	if (early_exit) return 0;

	if (!(nodes & 4095)) {
		// Check for early exit
		// We check every 4096 nodes to avoid slowing down the search too much
		uint64_t time = (clock() - start) / CLOCKS_PER_MS;
		if ((time > mxtime || nodes > mx_nodes) && exit_allowed) {
			early_exit = true;
			return 0;
		}
	}

	seldepth = std::max(depth, seldepth);
	Value stand_pat = eval(board) * side;
	apply_correction(board.side, board.pawn_struct_hash(), board.material_hash(), stand_pat);

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
	board.legal_moves(moves);

	// Sort captures and promotions
	pzstd::vector<std::pair<Move, Value>> scores;
	for (Move &move : moves) {
		if (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst())) {
			Value score = 0;
			score = MVV_LVA[board.mailbox[move.dst()] & 7][board.mailbox[move.src()] & 7];
			scores.push_back({move, score});
		} else if (move.type() == PROMOTION) {
			scores.push_back({move, PieceValue[move.promotion() + KNIGHT] - PawnValue});
		}
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) { return a.second > b.second; });

	Value best = stand_pat;

	for (int i = 0; i < scores.size(); i++) {
		Move &move = scores[i].first;

		if (move.type() != PROMOTION) {
			Value see = board.see_capture(move);
			if (see < 0) {
				continue; // Don't search moves that lose material
			} else {
				// QS Futility pruning
				// use see score for added safety
				if (DELTA_THRESHOLD + 4 * see + stand_pat < alpha) continue;
			}
		}

		board.make_move(move);
		Value score = -quiesce(board, -beta, -alpha, -side, depth + 1);
		board.unmake_move();

		if (score >= VALUE_MATE_MAX_PLY)
			score = score - (uint16_t(score >> 15) << 1) - 1; // Fixes "mate 0" bug

		if (score > best) {
			if (score > alpha)
				alpha = score;
			best = score;
		}
		if (score >= beta) {
			return best;
		}
	}

	return best;
}

/**
 * Order the moves based on various factors.
 * Move ordering priority:
 * 1. TTMove (highest priority)
 * 2. Captures + promotions (sorted by MVV+CaptHist)
 * 3. Quiets (sorted by history heuristic + counter-move heuristic + killer bonus)
 */
pzstd::vector<std::pair<Move, Value>> assign_values(Board &board, pzstd::vector<Move> &moves, int side, int depth, int ply, TTable::TTEntry *tentry) {
	pzstd::vector<std::pair<Move, Value>> scores;

	const Value TT_MOVE_BASE = VALUE_INFINITE;
	const Value CAPTURE_PROMO_BASE = 12000; // max value: base + mvv[queen] + max_history + promo = 12000 + 1002 + 16384 + 1002 = 30388
	const Value QUIET_BASE = -6000; // max value: base + max_history + cmh bonus = -6000 + 16384 + 1021 = 11405

	for (Move &move : moves) {
		// 1. TTMove must be first (don't do this outside loop because zobrist collisions can lead to illegal moves)
		if (tentry && move == tentry->best_move) {
			scores.push_back({move, TT_MOVE_BASE});
			continue;
		}

		bool capt = (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst()));
		bool promo = (move.type() == PROMOTION);

		Value score = 0;

		if (capt || promo) {
			// 2. Captures + promotions
			score = CAPTURE_PROMO_BASE;
			if (capt)
				score += PieceValue[board.mailbox[move.dst()] & 7] + capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
			if (promo)
				score += PieceValue[move.promotion() + KNIGHT] - PawnValue;
		} else {
			// 3. Quiets
			score = QUIET_BASE + history[board.side][move.src()][move.dst()];
			if (ply && move == cmh[board.side][line[ply-1].move.src()][line[ply-1].move.dst()]) {
				score += 1021; // Counter-move bonus
			}
			if (move == killer[0][ply]) {
				score += 1500; // Killer bonus
			} else if (move == killer[1][ply]) {
				score += 800; // Second killer bonus
			}
		}

		scores.push_back({move, score});
	}
	
	return scores;
}

Move next_move(pzstd::vector<std::pair<Move, Value>> &scores, int &end) {
	if (end == 0) return NullMove; // Ran out
	Move best_move = NullMove;
	Value best_score = -VALUE_INFINITE;
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

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1, bool pv = false, int ply = 1) {
	pvlen[ply] = 0;

	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return (VALUE_MATE) * side;
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return (-VALUE_MATE) * side;
	}

	// Control on white king and black king respectively
	auto wcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]));
	auto bcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]));
	
	if (board.side == WHITE) {
		// If it is white to move and white controls black's king, it's mate
		if (bcontrol.first > 0)
			return VALUE_MATE - 1;
	} else {
		// Likewise, the contrary also applies.
		if (wcontrol.second > 0)
			return VALUE_MATE - 1;
	}

	// Threefold or 50 move rule
	if (board.threefold() || board.halfmove >= 100) {
		return 0;
	}

	bool in_check = false;
	if (board.side == WHITE) {
		in_check = wcontrol.second > 0;
	} else {
		in_check = bcontrol.first > 0;
	}

	if (in_check) depth++; // Check extensions

	if (depth <= 0) {
		// Reached the maximum depth, perform quiescence search
		return quiesce(board, alpha, beta, side, ply);
	}

	// Check for TTable cutoff
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist);
	if (tentry && tentry->depth >= depth && line[ply].excl == NullMove) {
		// Check for cutoffs
		if (tentry->flags == EXACT) {
			return tentry->eval;
		} else if (tentry->flags == LOWER_BOUND && tentry->eval >= beta) {
			return tentry->eval;
		} else if (tentry->flags == UPPER_BOUND && tentry->eval <= alpha) {
			return tentry->eval;
		}
	}

	Value cur_eval = 0;
	Value raw_eval = 0; // For CorrHist
	Value mat_eval = simple_eval(board) * side; // For CorrHist
	uint64_t pawn_hash = 0;
	if (!in_check) {
		pawn_hash = board.pawn_struct_hash();
		cur_eval = tentry ? tentry->eval : eval(board) * side;
		raw_eval = cur_eval;
		apply_correction(board.side, pawn_hash, board.material_hash(), cur_eval);
	}

	line[ply].eval = in_check ? VALUE_NONE : cur_eval; // If in check, we don't have a valid eval yet

	bool improving = false;
	if (!in_check && ply >= 3 && line[ply-2].eval != VALUE_NONE && cur_eval > line[ply-2].eval) improving = true;

	// Reverse futility pruning
	if (!in_check && !pv) {
		/**
		 * The idea is that if we are winning by such a large margin that we can afford to lose
		 * RFP_THRESHOLD * depth eval units per ply, we can return the current eval.
		 * 
		 * We need to make sure that we aren't in check (since we might get mated)
		 */
		int margin = (RFP_THRESHOLD - improving * RFP_IMPROVING) * depth;
		if (cur_eval >= beta + margin)
			return cur_eval - margin;
	}

	// Null-move pruning
	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	int npawns_and_kings = _mm_popcnt_u64(board.piece_boards[PAWN] | board.piece_boards[KING]);
	if (!in_check && npieces != npawns_and_kings && cur_eval >= beta) { // Avoid NMP in pawn endgames
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
		Value null_score = -__recurse(board, depth - NMP_R_VALUE, -beta, -beta + 1, -side, pv, ply+1);
		board.unmake_move();
		if (null_score >= beta)
			return null_score;
	}

	// Razoring
	if (!pv && !in_check && depth <= 3 && cur_eval + RAZOR_MARGIN * depth < alpha) {
		/**
		 * If we are losing by a lot, check w/ qsearch to see if we could possibly improve.
		 * If not, we can prune the search.
		 */
		Value razor_score = quiesce(board, alpha, beta, side, ply);
		if (razor_score < alpha)
			return razor_score;
	}

	Value best = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	pzstd::vector<std::pair<Move, Value>> scores = assign_values(board, moves, side, depth, ply, tentry);
	int end = scores.size();

	if (depth > 4 && !tentry) {
		depth -= 2; // Internal iterative reductions
	}

	Move best_move = NullMove;

	pzstd::vector<Move> quiets, captures;

	Move move = NullMove;
	int i = 0;
	int extension = 0;

	while ((move = next_move(scores, end)) != NullMove) {
		if (move == line[ply].excl) {
			i++; // Necessary so we don't do full search
			continue;
		}

		bool capt = (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst()));
		bool promo = (move.type() == PROMOTION);
		
		if (line[ply].excl == NullMove && depth >= 8 && tentry && move == tentry->best_move && tentry->depth >= depth - 2 && tentry->flags != UPPER_BOUND) {
			// Singular extension
			line[ply].excl = move;
			Value singular_beta = tentry->eval - 6 * depth;
			Value singular_score = __recurse(board, (depth-1) / 2, singular_beta - 1, singular_beta, side, 0, ply);
			line[ply].excl = NullMove; // Reset exclusion move

			if (singular_score < singular_beta) {
				extension++;
			} else if (tentry->eval >= beta) {
				// Negative extensions
				extension -= 3;
			}
		}

		line[ply].move = move;

		if (!in_check && !capt && !promo && i > 5 + 2 * depth * depth) {
			/**
			 * Late Move Pruning
			 * 
			 * Just skip later moves that probably aren't good
			 */
			continue;
		}

		if (!in_check && !capt && !promo && depth <= 5) {
			/**
			 * History pruning
			 * 
			 * Skip moves with very bad history scores
			 * Depth condition is necessary to avoid overflow
			 */
			Value hist = history[board.side][move.src()][move.dst()];
			if (hist < -HISTORY_MARGIN * depth) {
				continue;
			}
		}

		if (depth <= 2 && i > 0 && !in_check && !capt && !promo && abs(alpha) < VALUE_MATE_MAX_PLY && abs(beta) < VALUE_MATE_MAX_PLY) {
			/**
			 * Futility pruning
			 * 
			 * If we are at the leaf of the search, we can prune moves that are
			 * probably not going to be better than alpha.
			 */
			if (depth == 1 && cur_eval + FUTILITY_THRESHOLD < alpha) continue;
			if (depth == 2 && cur_eval + FUTILITY_THRESHOLD2 < alpha) continue;
		}

		board.make_move(move);

		Value score;
		if (i > 0) {
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
			Value r = reduction[i][depth];
			r -= 512 * pv;
			if (tentry && (board.piece_boards[OCC(board.side)] & square_bits(tentry->best_move.dst())))
				// reduce more if tentry is a capture
				r += 800;
			if (r < 1024) r = 1024; // ensure at least 1 ply reduction
			score = -__recurse(board, depth - r / 1024, -alpha - 1, -alpha, -side, 0, ply+1);
			if (score > alpha) {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side, pv, ply+1);
			}
		} else {
			score = -__recurse(board, depth - 1 + extension, -beta, -alpha, -side, pv, ply+1);
		}

		if (abs(score) >= VALUE_MATE_MAX_PLY)
			score = score - (uint16_t(score >> 15) << 1) - 1; // Mate score fix

		board.unmake_move();

		if (score > best) {
			if (score > alpha) {
				alpha = score;
				if (score < beta) {
					pvtable[ply][0] = move;
					pvlen[ply] = pvlen[ply+1]+1;
					for (int i = 0; i < pvlen[ply+1]; i++) {
						pvtable[ply][i+1] = pvtable[ply+1][i];
					}
				}
			}
			best = score;
			best_move = move;
		}

		if (score >= beta) {
			if (line[ply].excl == NullMove) {
				board.ttable.store(board.zobrist, best, depth, LOWER_BOUND, best_move, board.halfmove);
			}
			if (killer[0][ply] != move) {
				killer[1][ply] = killer[0][ply];
				killer[0][ply] = move; // Update killer moves
			}
			if (!capt) { // Not a capture
				const Value bonus = 1.56 * depth * depth + 0.91 * depth + 0.62;
				update_history(board.side, move.src(), move.dst(), bonus);
				for (auto &qmove : quiets) {
					update_history(board.side, qmove.src(), qmove.dst(), -bonus); // Penalize quiet moves
				}
				cmh[board.side][line[ply-1].move.src()][line[ply-1].move.dst()] = move; // Update counter-move history
				if (!in_check && !promo && best > raw_eval) update_corrhist(board.side, pawn_hash, board.material_hash(), best - raw_eval, depth);
			} else { // Capture
				const Value bonus = 1.81 * depth * depth + 0.52 * depth + 0.40;
				update_capthist(PieceType(board.mailbox[move.src()] & 7), PieceType(board.mailbox[move.dst()] & 7), move.dst(), bonus);
				for (auto &cmove : captures) {
					update_capthist(PieceType(board.mailbox[cmove.src()] & 7), PieceType(board.mailbox[cmove.dst()] & 7), cmove.dst(), -bonus);
				}
			}
			return best;
		}

		if (early_exit)
			break;

		if (!capt && !promo) quiets.push_back(move);
		else if (capt) captures.push_back(move);
		i++;
	}

	bool best_iscapture = (board.piece_boards[OPPOCC(board.side)] & square_bits(best_move.dst()));
	bool best_ispromo = (best_move.type() == PROMOTION);
	if (!in_check && !best_iscapture && !best_ispromo && !(best < alpha && best >= raw_eval)) {
		// Best move is a quiet move, update CorrHist
		update_corrhist(board.side, pawn_hash, board.material_hash(), best - raw_eval, depth);
	}

	// Stalemate detection
	if (best == -VALUE_MATE + 2) {
		// If our engine thinks we are mated but we are not in check, we are stalemated
		if (board.side == WHITE) {
			if (!board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second)
				best = 0;
		} else {
			if (!board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first)
				best = 0;
		}
	}

	if (line[ply].excl == NullMove) {
		if (best <= alpha) {
			board.ttable.store(board.zobrist, alpha, depth, UPPER_BOUND, best_move, board.halfmove);
		} else {
			board.ttable.store(board.zobrist, best, depth, EXACT, best_move, board.halfmove);
		}
	}

	return best;
}

// Search function from the first layer of moves
std::pair<Move, Value> __search(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	Move best_move = NullMove;
	Value best_score = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist);
	pzstd::vector<std::pair<Move, Value>> scores = assign_values(board, moves, side, depth, 0, tentry);

	Move move = NullMove;
	int end = scores.size();
	int i = 0;
	while ((move = next_move(scores, end)) != NullMove) {
		if (depth >= 20 && nodes >= 10'000'000) {
			std::cout << "info depth " << depth << " currmove " << move.to_string() << " currmovenumber " << i+1 << std::endl;
		}

		line[0].move = move;
		board.make_move(move);
		Value score;
		if (i > 0) {
			score = -__recurse(board, depth - reduction[i][depth] / 1024, -alpha - 1, -alpha, -side, 0);
			if (score > alpha) {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side, 0);
			}
		} else {
			score = -__recurse(board, depth - 1, -beta, -alpha, -side, 1);
		}

		board.unmake_move();

		if (score > best_score) {
			pvtable[0][0] = move;
			pvlen[0] = pvlen[1]+1;
			for (int i = 0; i < pvlen[1]; i++) {
				pvtable[0][i+1] = pvtable[1][i];
			}
			if (score > alpha) {
				alpha = score;
			}
			best_score = score;
			best_move = move;
		}

		if (score >= beta) {
			board.ttable.store(board.zobrist, best_score, depth, LOWER_BOUND, best_move, board.halfmove);
			if (killer[0][0] != move) {
				killer[1][0] = killer[0][0];
				killer[0][0] = move;
			}
			return {best_move, best_score};
		}

		if (early_exit)
			break;

		i++;
	}

	if (best_score <= alpha) {
		board.ttable.store(board.zobrist, alpha, depth, UPPER_BOUND, best_move, board.halfmove);
	} else {
		board.ttable.store(board.zobrist, best_score, depth, EXACT, best_move, board.halfmove);
	}

	return {best_move, best_score};
}

void __print_pv(bool omit_last = 0) { // Need to omit last to prevent illegal moves during mates
	const int ROOT_PLY = 0;
	for (int i = 0; i < pvlen[ROOT_PLY] - omit_last; i++) {
		if (pvtable[ROOT_PLY][i] == NullMove) break;
		std::cout << pvtable[ROOT_PLY][i].to_string() << ' ';
	}
}

std::pair<Move, Value> search(Board &board, int64_t time, bool quiet) {
	std::cout << std::fixed << std::setprecision(0);
	nodes = seldepth = 0;
	early_exit = exit_allowed = false;
	start = clock();
	mxtime = time;
	
	// Clear killer moves and history heuristic
	for (int i = 0; i < MAX_PLY; i++) {
		killer[0][i] = killer[1][i] = NullMove;
		pvlen[i] = 0;
	}

	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			history[0][i][j] = history[1][i][j] = 0;
		}
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < CORRHIST_SZ; j++) {
			corrhist_ps[i][j] = 0;
		}
	}

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	bool aspiration_enabled = true;
	for (int d = 1; d <= MAX_PLY; d++) {
		Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		Value window_size = ASPIRATION_WINDOW;
		
		if (eval != -VALUE_INFINITE && aspiration_enabled) {
			/**
			 * Aspiration windows work by searching a small window around the expected value
			 * of the position. By having a smaller window, our search runs faster. 
			 * 
			 * If we fail either high or low out of this window, we gradually expand the
			 * window size, eventually getting to a full-width search.
			 */
			alpha = eval - window_size;
			beta = eval + window_size;
		}
		
		auto result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		
		// Gradually expand the window if we fail high or low
		while ((result.second >= beta || result.second <= alpha) && window_size < VALUE_INFINITE / 4) {
			if (result.second >= beta) {
				// Fail high - expand upper bound
				beta = eval + window_size * 2;
				if (beta >= VALUE_INFINITE / 4) beta = VALUE_INFINITE;
			}
			if (result.second <= alpha) {
				// Fail low - expand lower bound  
				alpha = eval - window_size * 2;
				if (alpha <= -VALUE_INFINITE / 4) alpha = -VALUE_INFINITE;
			}
			window_size *= 2;
			result = __search(board, d, alpha, beta, board.side ? -1 : 1);
			if (early_exit) break;
		}
		if (early_exit) break;
		eval = result.second;
		best_move = result.first;
		
		seldepth = std::max(seldepth, d);
		
		#ifndef NOUCI
		if (!quiet) {
			if (abs(eval) >= VALUE_MATE_MAX_PLY) {
				std::cout << "info depth " << d << " seldepth " << seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval > 0 ? 1 : -1) << " nodes "
				<< nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv ";
				__print_pv(1);
				std::cout << "hashfull " << (board.ttable.size() * 1000 / board.ttable.mxsize()) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
			} else {
				std::cout << "info depth " << d << " seldepth " << seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps "
				<< (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv ";
				__print_pv();
				std::cout << "hashfull " << (board.ttable.size() * 1000 / board.ttable.mxsize()) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
			}
		}
		#endif
		
		exit_allowed = true;
		
		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			return {best_move, eval};
			// We don't need to search further, we found mate
		}

		int time_elapsed = (clock() - start) / CLOCKS_PER_MS;
		if (time_elapsed > mxtime * 0.5) {
			// We probably won't be able to complete the next ID loop
			break;
		}
	}

	return {best_move, eval / CP_SCALE_FACTOR};
}

std::pair<Move, Value> search_depth(Board &board, int depth, bool quiet) {
	mx_nodes = 1e18;
	std::cout << std::fixed << std::setprecision(0);
	nodes = seldepth = 0;
	early_exit = exit_allowed = false;
	start = clock();

	// Clear killer moves and history heuristic
	for (int i = 0; i < MAX_PLY; i++) {
		killer[0][i] = killer[1][i] = NullMove;
		pvlen[i] = 0;
	}

	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			history[0][i][j] = history[1][i][j] = 0;
		}
	}

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	bool aspiration_enabled = true;
	for (int d = 1; d <= depth; d++) {
		Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		Value window_size = ASPIRATION_WINDOW;
		
		if (eval != -VALUE_INFINITE && aspiration_enabled) {
			alpha = eval - window_size;
			beta = eval + window_size;
		}
		
		auto result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		
		// Gradually expand the window if we fail high or low
		while ((result.second >= beta || result.second <= alpha) && window_size < VALUE_INFINITE / 4) {
			if (result.second >= beta) {
				// Fail high - expand upper bound
				beta = eval + window_size * 2;
				if (beta >= VALUE_INFINITE / 4) beta = VALUE_INFINITE;
			}
			if (result.second <= alpha) {
				// Fail low - expand lower bound
				alpha = eval - window_size * 2;
				if (alpha <= -VALUE_INFINITE / 4) alpha = -VALUE_INFINITE;
			}
			window_size *= 2;
			result = __search(board, d, alpha, beta, board.side ? -1 : 1);
			if (early_exit) break;
		}
		if (early_exit)
			break;
		eval = result.second;
		best_move = result.first;

		seldepth = std::max(seldepth, d);

#ifndef NOUCI
		if (!quiet) {
			if (abs(eval) >= VALUE_MATE_MAX_PLY) {
				std::cout << "info depth " << d << " seldepth " << seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval > 0 ? 1 : -1) << " nodes "
						<< nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv ";
				__print_pv(1);
				std::cout << "hashfull " << (board.ttable.size() * 1000 / board.ttable.mxsize()) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
			} else {
				std::cout << "info depth " << d << " seldepth " << seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps "
						<< (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv ";
				__print_pv();
				std::cout << "hashfull " << (board.ttable.size() * 1000 / board.ttable.mxsize()) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
			}
		}
#endif
	}

	return {best_move, eval / CP_SCALE_FACTOR};
}

std::pair<Move, Value> search_nodes(Board &board, uint64_t nodes) {
	mx_nodes = nodes;
	auto res = search(board);
	return res;
} // Search for a given number of nodes
