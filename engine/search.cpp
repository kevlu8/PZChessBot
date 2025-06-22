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
 * Currently, the function is very gentle because our move ordering is not ideal.
 * See https://www.chessprogramming.org/Late_Move_Reductions
 */
uint16_t reduction(int i, int d) {
	if (d <= 1 || i <= 1)
		return 1; // Don't reduce on nodes that lead to leaves since the TT doesn't provide info
	return 0.77 + log2(i) * log2(d) / 2.36;
}

/**
 * MVV_LVA (most-valuable-victim:least-valuable-attacker) is a metric for move ordering that helps
 * sort captures and promotions. We basically sort high-value captures first, and low-value captures
 * last.
 * 
 * This is currently not used because somehow static eval sorting is outperforming it.
 */
Value MVV_LVA[6][6];

__attribute__((constructor)) void init_mvvlva() {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			if (i == KING)
				MVV_LVA[i][j] = QueenValue * 12 + 1; // Prioritize over all other captures
			else
				MVV_LVA[i][j] = PieceValue[i] * 12 - PieceValue[j];
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
 * 
 * TODO: check if overflows are possible
 */
Value history[2][64][64];
Value capthist[6][6][64]; // [piece][captured piece][dst]

/**
 * The counter-move heuristic is a move ordering heuristic that helps sort moves that
 * have refuted other moves in the past. It works by storing the move upon a beta cutoff.
 */
Move cmh[2][64][64];

Move line[MAX_PLY]; // Currently searched line

Move pvtable[MAX_PLY][MAX_PLY];
int pvlen[MAX_PLY];

void update_history(bool side, Square from, Square to, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	history[side][from][to] += cbonus - history[side][from][to] * abs(bonus) / MAX_HISTORY;
}

void update_capthist(PieceType piece, PieceType captured, Square dst, Value bonus) {
	int cbonus = std::clamp(bonus, (Value)(-MAX_HISTORY), MAX_HISTORY);
	capthist[piece][captured][dst] += cbonus - capthist[piece][captured][dst] * abs(bonus) / MAX_HISTORY;
}

/**
 * Perform the quiescence search
 * 
 * Quiescence search is a technique to avoid the horizon effect, where the evaluation function
 * incorrectly evaluates a position because it is not stable (e.g. there is a hanging queen).
 * In this function, we search only captures and promotions, and return the best score.
 * 
 * TODO:
 * - Search for checks and check evasions
 * - Delta pruning (sort of like futility pruning, see https://www.chessprogramming.org/Delta_Pruning)
 * - Late move reduction (instead of reducing depth, we reduce the search window)
 * - Static exchange evaluation (don't search moves that lose material, see https://www.chessprogramming.org/Static_Exchange_Evaluation)
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

	// If it's a mate, stop here since there's no point in searching further
	if (stand_pat == VALUE_MATE || stand_pat == -VALUE_MATE)
		return stand_pat;

	// If we are too good, return the score
	if (stand_pat >= beta)
		return stand_pat;
	if (stand_pat > alpha)
		alpha = stand_pat;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	// Sort captures and promotions (ideally we should be using MVV_LVA here)
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

		// Value see = board.see_capture(move);
		// if (see < 0) {
		// 	continue; // Don't search moves that lose material
		// }

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
 * First, we check if we have a TTable entry for this position. If we do, we add it to the
 * beginning of the list, since it is almost definitely the best move. Then, the other moves
 * are sorted based on:
 * - MVV_LVA (most valuable victim, least valuable attacker) for captures
 * - Piece value for promotions
 * - History heuristic for quiet moves
 * - Killer moves (moves that have caused a beta cutoff in the past)
 * - Counter-move history (moves that have refuted other moves in the past)
 */
pzstd::vector<std::pair<Move, Value>> order_moves(Board &board, pzstd::vector<Move> &moves, int side, int depth, int ply, bool &entry_exists) {
	pzstd::vector<std::pair<Move, Value>> scores;
	// If we have a TTable entry *at all* for this position, we should use it
	// Even if it falls outside of our alpha-beta window, it probably provides a decent move
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, VALUE_INFINITE, -VALUE_INFINITE, -1);
	Move entry = tentry ? tentry->best_move : NullMove;
	entry_exists = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
		entry_exists = true;
	}
	for (Move &move : moves) {
		if (move == entry) continue; // Don't add the TT move again
		Value score = 0;
		if (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst())) {
			// score = MVV_LVA[board.mailbox[move.dst()] & 7][board.mailbox[move.src()] & 7];
			score = PieceValue[board.mailbox[move.dst()] & 7] + capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
		} else if (move.type() == PROMOTION) {
			score = PieceValue[move.promotion() + KNIGHT] - PawnValue;
		} else {
			// Non-capture, non-promotion, so check history
			score = history[board.side][move.src()][move.dst()];
		}
		if (move == killer[0][depth]) {
			score += 1500; // Killer move bonus
		} else if (move == killer[1][depth]) {
			score += 800; // Second killer move bonus
		}
		if (ply && move == cmh[board.side][line[ply-1].src()][line[ply-1].dst()]) {
			score += 1000; // Counter-move bonus
		}
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) { return a.second > b.second; });
	return scores;
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

	// Control on white king and black king respectively. First is white's control, second is that of black
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
	TTable::TTEntry *cutoff = board.ttable.probe(board.zobrist, alpha, beta, depth);
	if (cutoff)
		return cutoff->eval;

	Value cur_eval = 0;
	if (!in_check) cur_eval = eval(board) * side;

	// Reverse futility pruning
	if (!in_check && !pv && depth <= 3) {
		/**
		 * The idea is that if we are winning by such a large margin that we can afford to lose
		 * RFP_THRESHOLD * depth eval units per ply, we can return the current eval.
		 * 
		 * We need to make sure that we aren't in check (since we might get mated) and that the
		 * TT entry exists (so that the current position is actually good).
		 */
		int margin = RFP_THRESHOLD * depth;
		if (cur_eval >= beta + margin)
			return cur_eval - margin;
	}

	// Null-move pruning
	if (!in_check && _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) >= 8) {
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

	Value best = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	bool entry_exists = false;
	pzstd::vector<std::pair<Move, Value>> scores = order_moves(board, moves, side, depth, ply, entry_exists);

	if (depth > 5 && !entry_exists) {
		depth -= 2; // Internal iterative reductions
	}

	Move best_move = NullMove;

	pzstd::vector<Move> quiets, captures;

	for (int i = 0; i < moves.size(); i++) {
		Move &move = scores[i].first;
		line[ply] = move;

		bool capt = (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst()));
		bool promo = (move.type() == PROMOTION);

		if (depth == 1 && i > 0 && !in_check && !capt && !promo && abs(alpha) < VALUE_MATE_MAX_PLY && abs(beta) < VALUE_MATE_MAX_PLY) {
			/**
			 * Futility pruning
			 * 
			 * If we are at the leaf of the search, we can prune moves that are
			 * probably not going to be better than alpha.
			 */
			if (cur_eval + FUTILITY_THRESHOLD < alpha) continue;
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
			score = -__recurse(board, depth - reduction(i, depth), -alpha - 1, -alpha, -side, 0, ply+1);
			if (score > alpha) {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side, pv, ply+1);
			}
		} else {
			score = -__recurse(board, depth - 1, -beta, -alpha, -side, pv, ply+1);
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
			board.ttable.store(board.zobrist, best, depth, LOWER_BOUND, best_move, board.halfmove);
			if (killer[0][depth] != move) {
				killer[1][depth] = killer[0][depth];
				killer[0][depth] = move; // Update killer moves
			}
			if (!(board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst()))) { // Not a capture
				const Value bonus = depth * depth;
				update_history(board.side, move.src(), move.dst(), bonus);
				for (auto &qmove : quiets) {
					update_history(board.side, qmove.src(), qmove.dst(), -bonus); // Penalize quiet moves
				}
				cmh[board.side][line[ply-1].src()][line[ply-1].dst()] = move; // Update counter-move history
			} else {
				const Value bonus = depth * depth;
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

	if (best <= alpha) {
		board.ttable.store(board.zobrist, alpha, depth, UPPER_BOUND, best_move, board.halfmove);
	} else {
		board.ttable.store(board.zobrist, best, depth, EXACT, best_move, board.halfmove);
	}

	return best;
}

// Search function from the first layer of moves
std::pair<Move, Value> __search(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	Move best_move = NullMove;
	Value best_score = -VALUE_INFINITE;

	int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
	bool use_egnn = npieces <= 10;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	bool entry_exists = false;
	pzstd::vector<std::pair<Move, Value>> scores = order_moves(board, moves, side, depth, 0, entry_exists);

	for (int i = 0; i < moves.size(); i++) { // Skip the TT move if it's not legal
		Move &move = scores[i].first;
		line[0] = move;
		board.make_move(move);
		Value score;
		if (i > 0) {
			score = -__recurse(board, depth - reduction(i, depth), -alpha - 1, -alpha, -side, 0);
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
			if (killer[0][depth] != move) {
				killer[1][depth] = killer[0][depth];
				killer[0][depth] = move;
			}
			return {best_move, best_score};
		}

		if (early_exit)
			break;
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
		if (time_elapsed > mxtime / 2) {
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
