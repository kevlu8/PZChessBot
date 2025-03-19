#include "search.hpp"

#define MOVENUM(x) ((((#x)[1] - '1') << 12) | (((#x)[0] - 'a') << 8) | (((#x)[3] - '1') << 4) | ((#x)[2] - 'a'))

uint64_t nodes = 0;
int seldepth = 0;
uint64_t mxtime = 1000;
bool early_exit = false, exit_allowed = false;
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

uint16_t reduction(int i, int d) {
	if (d <= 1 || i <= 1)
		return 1; // Don't reduce on nodes that lead to leaves since the TT doesn't provide info
	// return 0.77 + log2(i) * log2(d) / 2.36;
	if (i > 24) return 3;
	if (i > 12) return 2;
	return 1;
}

Value quiesce(Board &board, Value alpha, Value beta, int side, int depth) {
	nodes++;

	if (early_exit) return 0;

	if (!(nodes & 4095)) {
		// Check for early exit
		uint64_t time = (clock() - start) / CLOCKS_PER_MS;
		if (time > mxtime && exit_allowed) {
			early_exit = true;
			return 0;
		}
	}

	seldepth = std::max(depth, seldepth);
	Value stand_pat = eval(board) * side;

	if (stand_pat == VALUE_MATE || stand_pat == -VALUE_MATE)
		return stand_pat;

	if (stand_pat >= beta)
		return stand_pat;
	if (stand_pat > alpha)
		alpha = stand_pat;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	pzstd::vector<std::pair<Move, Value>> scores;
	for (Move &move : moves) {
		if (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst())) {
			// Value score = 0;
			// score = MVV_LVA[board.mailbox[move.dst()] & 7][board.mailbox[move.src()] & 7];
			// scores.push_back({move, score});
			board.make_move(move);
			Value score = eval(board) * side;
			board.unmake_move();
			scores.push_back({move, score});
		} else if (move.type() == PROMOTION) {
			// scores.push_back({move, PieceValue[move.promotion() + KNIGHT] - PawnValue});
			board.make_move(move);
			Value score = eval(board) * side;
			board.unmake_move();
			scores.push_back({move, score});
		}
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) { return a.second > b.second; });

	Value best = stand_pat;

	for (int i = 0; i < scores.size(); i++) {
		Move &move = scores[i].first;
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

pzstd::vector<std::pair<Move, Value>> order_moves(Board &board, pzstd::vector<Move> &moves, int side, int depth, bool &entry_exists) {
	pzstd::vector<std::pair<Move, Value>> scores;
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, VALUE_INFINITE, -VALUE_INFINITE, -1);
	Move entry = tentry ? tentry->best_move : NullMove;
	entry_exists = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
		entry_exists = true;
	}
	for (Move &move : moves) {
		if (move == entry) continue; // Don't add the TT move again
		board.make_move(move);
		Value score = eval(board) * side;
		board.unmake_move();
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) { return a.second > b.second; });
	return scores;
}

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1, bool pv = false) {
	if (depth <= 0) {
		return quiesce(board, alpha, beta, side, 0);
	}

	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return (VALUE_MATE) * side;
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return (-VALUE_MATE) * side;
	}

	auto wcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]));
	auto bcontrol = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]));
	
	if (board.side == WHITE) {
		if (bcontrol.first > 0)
			return VALUE_MATE - 1;
	} else {
		if (wcontrol.second > 0)
			return VALUE_MATE - 1;
	}

	// Check for TTable cutoff
	TTable::TTEntry *cutoff = board.ttable.probe(board.zobrist, alpha, beta, depth);
	if (cutoff) {
		return cutoff->eval;
	}

	bool in_check = false;
	if (board.side == WHITE) {
		in_check = wcontrol.second > 0;
	} else {
		in_check = bcontrol.first > 0;
	}

	if (!in_check) {
		// Perform null-move pruning
		board.make_move(NullMove);
		Value null_score = -__recurse(board, depth - 3, -beta, -beta + 1, -side, pv);
		board.unmake_move();
		if (null_score >= beta)
			return null_score;
	}

	Value best = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	bool entry_exists = false;
	pzstd::vector<std::pair<Move, Value>> scores = order_moves(board, moves, side, depth, entry_exists);

	// Reverse futility pruning
	if (!in_check && entry_exists && !pv) {
		int cur_eval = eval(board) * side;
		int margin = RFP_THRESHOLD * depth;
		if (cur_eval >= beta + margin)
			return cur_eval;
	}

	Move best_move = NullMove;

	for (int i = 0; i < moves.size(); i++) {
		Move &move = scores[i].first;
		board.make_move(move);

		Value score;
		if (board.threefold()) {
			score = 0; // Draw by repetition
			// Make sure we're not in check or this is an illegal move
			if (board.side == WHITE) {
				// Black just played a move, so we check that white doesn't control black's king
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first)
					score = -VALUE_MATE;
			} else {
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second)
					score = -VALUE_MATE;
			}
		} else {
			if (i > 3) {
				score = -__recurse(board, depth - reduction(i, depth), -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth - 1, -beta, -alpha, -side);
				}
			} else {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side, 1);
			}
		}

		if (abs(score) >= VALUE_MATE_MAX_PLY)
			score = score - (uint16_t(score >> 15) << 1) - 1;

		board.unmake_move();

		if (score > best) {
			if (score > alpha) {
				alpha = score;
			}
			best = score;
			best_move = move;
		}
		if (score >= beta) {
			board.ttable.store(board.zobrist, best, depth, LOWER_BOUND, best_move, board.halfmove);
			return best;
		}

		if (early_exit)
			break;
	}

	// Stalemate detection
	if (best == -VALUE_MATE + 2) {
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

std::pair<Move, Value> __search(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	Move best_move = NullMove;
	Value best_score = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	bool entry_exists = false;
	pzstd::vector<std::pair<Move, Value>> scores = order_moves(board, moves, side, depth, entry_exists);

	for (int i = 0; i < moves.size(); i++) { // Skip the TT move if it's not legal
		Move &move = scores[i].first;
		board.make_move(move);
		Value score;
		if (board.threefold()) {
			score = 0; // Draw by repetition
			// Make sure we're not in check or this is an illegal move
			if (board.side == WHITE) {
				// Black just played a move, so we check that white doesn't control black's king
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first)
					score = -VALUE_MATE;
			} else {
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second)
					score = -VALUE_MATE;
			}
		} else {
			if (i > 3) {
				score = -__recurse(board, depth - reduction(i, depth), -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth - 1, -beta, -alpha, -side);
				}
			} else {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side, 1);
			}
		}

		board.unmake_move();

		if (score > best_score) {
			if (score > alpha) {
				alpha = score;
			}
			best_score = score;
			best_move = move;
		}

		if (score >= beta) {
			board.ttable.store(board.zobrist, best_score, depth, LOWER_BOUND, best_move, board.halfmove);
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

std::pair<Move, Value> search(Board &board, int64_t time) {
	std::cout << std::fixed << std::setprecision(0);
	nodes = seldepth = 0;
	early_exit = exit_allowed = false;
	start = clock();
	mxtime = time;

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	bool aspiration_enabled = true;
	for (int d = 1; d <= MAX_PLY; d++) {
		Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		if (eval != -VALUE_INFINITE && aspiration_enabled) {
			// Aspiration window values are rather large because our eval function
			// does not return values in centipawns, but rather in closer to ~1/400
			// So this window size is actually around 50 centipawns
			alpha = eval - ASPIRATION_WINDOW;
			beta = eval + ASPIRATION_WINDOW;
		}
		auto result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		// Check for fail-high or fail-low
		bool research = result.second >= beta || result.second <= alpha;
		if (result.second >= beta) {
			beta = VALUE_INFINITE;
		}
		if (result.second <= alpha) {
			alpha = -VALUE_INFINITE;
		}
		if (research) {
			result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		}
		if (early_exit)
			break;
		eval = result.second;
		best_move = result.first;

		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval > 0 ? 1 : -1) << " nodes "
					  << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv " << best_move.to_string() << " hashfull "
					  << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		} else {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps "
					  << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv " << best_move.to_string() << " hashfull "
					  << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		}

		exit_allowed = true;

		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			return {best_move, eval};
			// We don't need to search further, we found mate
		}
	}

	return {best_move, eval / CP_SCALE_FACTOR};
}

std::pair<Move, Value> search_depth(Board &board, int depth) {
	std::cout << std::fixed << std::setprecision(0);
	nodes = seldepth = 0;
	early_exit = exit_allowed = false;
	start = clock();

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	bool aspiration_enabled = true;
	for (int d = 1; d <= depth; d++) {
		Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
		if (eval != -VALUE_INFINITE && aspiration_enabled) {
			// Aspiration window values are rather large because our eval function
			// does not return values in centipawns, but rather in closer to ~1/400
			// So this window size is actually around 50 centipawns
			alpha = eval - ASPIRATION_WINDOW;
			beta = eval + ASPIRATION_WINDOW;
		}
		auto result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		// Check for fail-high or fail-low
		bool research = result.second >= beta || result.second <= alpha;
		if (result.second >= beta) {
			beta = VALUE_INFINITE;
		}
		if (result.second <= alpha) {
			alpha = -VALUE_INFINITE;
		}
		if (research) {
			result = __search(board, d, alpha, beta, board.side ? -1 : 1);
		}
		if (early_exit)
			break;
		eval = result.second;
		best_move = result.first;

		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval > 0 ? 1 : -1) << " nodes "
					  << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv " << best_move.to_string() << " hashfull "
					  << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		} else {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps "
					  << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " pv " << best_move.to_string() << " hashfull "
					  << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		}
	}

	return {best_move, eval / CP_SCALE_FACTOR};
}
