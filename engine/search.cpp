#include "search.hpp"

#define MOVENUM(x) ((((#x)[1]-'1') << 12) | (((#x)[0]-'a') << 8) | (((#x)[3]-'1') << 4) | ((#x)[2]-'a'))

uint64_t nodes = 0;
int seldepth = 0, mxtime = 1000;
bool early_exit = false, exit_allowed = false;
clock_t start = 0;

uint64_t perft(Board &board, int depth) {
	// If white's turn is beginning and black is in check
	if (board.side == WHITE && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first)
		return 0;
	// If black's turn is beginning and white is in check
	else if (board.side == BLACK && board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second)
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
	if (d <= 1 || i <= 1) return 1; // Don't reduce on nodes that lead to leaves since the TT doesn't provide info
	if (i > 31) return 3;
	if (i > 15) return 2;
	return 1;
}

Value MVV_LVA[6][6]; // [vctm][atk]

__attribute__((constructor)) void init_mvvlva() {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			MVV_LVA[i][j] = PieceValue[i] - PieceValue[j];
		}
	}
}

Value quiesce(Board &board, Value alpha, Value beta, int side, int depth) {
	nodes++;

	if (!(nodes & 32767)) {
		// Check for early exit
		int time = (clock() - start) / CLOCKS_PER_MS;
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
			Value score = 0;
			score = MVV_LVA[board.mailbox[move.dst()] & 7][board.mailbox[move.src()] & 7];
			scores.push_back({move, score});
		} else if (move.type() == PROMOTION) {
			scores.push_back({move, PieceValue[move.promotion() + KNIGHT] - PawnValue});
		}
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
		return a.second > b.second;
	});

	Value best = stand_pat;

	for (int i = 0; i < scores.size(); i++) {
		Move &move = scores[i].first;
		board.make_move(move);
		Value score = -quiesce(board, -beta, -alpha, -side, depth+1);
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

	// Check for TTable cutoff
	TTable::TTEntry *cutoff = board.ttable.probe(board.zobrist, alpha, beta, depth);
	if (cutoff) {
		return cutoff->eval;
	}

	bool in_check = false;
	if (board.side == WHITE) {
		in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])).second;
	} else {
		in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])).first;
	}

	Value best = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	// Move ordering (experimental)
	pzstd::vector<std::pair<Move, Value>> scores;
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, VALUE_INFINITE, -VALUE_INFINITE, -1);
	Move entry = tentry ? tentry->best_move : NullMove;
	bool entry_is_legal = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
	} else entry_is_legal = true; // Don't skip the first move if we never added it
	for (Move &move : moves) {
		if (move == entry) {
			entry_is_legal = true;
			continue; // Don't add the TT move again
		}
		Value score = 0;
		board.make_move(move);
		score = eval(board) * side;
		board.unmake_move();
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
		return a.second > b.second;
	});
	for (int i = 0; i < scores.size(); i++) moves[i] = scores[i].first;

	// Reverse futility pruning
	if (!in_check && entry_is_legal && entry != NullMove && !pv) {
		int cur_eval = eval(board) * side;
		int margin = 400 * depth;
		if (cur_eval >= beta + margin)
			return cur_eval;
	}

	Move best_move = NullMove;

	for (int i = !entry_is_legal; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);

		Value score;
		if (board.threefold()) {
			score = 0; // Draw by repetition
			// Make sure we're not in check or this is an illegal move
			if (board.side == WHITE) {
				// Black just played a move, so we check that white doesn't control black's king
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])).first)
					score = -VALUE_MATE;
			} else {
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])).second)
					score = -VALUE_MATE;
			}
		} else {
			if (i > 3) {
				score = -__recurse(board, depth-reduction(i, depth), -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth-1, -beta, -alpha, -side);
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
			if (!board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])).second)
				best = 0;
		} else {
			if (!board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])).first)
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

	pzstd::vector<std::pair<Move, Value>> scores;
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, VALUE_INFINITE, -VALUE_INFINITE, -1);
	Move entry = tentry ? tentry->best_move : NullMove;
	bool entry_is_legal = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
	} else entry_is_legal = true; // Don't skip the first move if we never added it
	for (Move &move : moves) {
		if (move == entry) {
			entry_is_legal = true;
			continue; // Don't add the TT move again
		}
		Value score = 0;
		board.make_move(move);
		score = eval(board) * side;
		board.unmake_move();
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
		return a.second > b.second;
	});
	for (int i = 0; i < scores.size(); i++) moves[i] = scores[i].first;

	for (int i = !entry_is_legal; i < moves.size(); i++) { // Skip the TT move if it's not legal
		Move &move = moves[i];
		board.make_move(move);
		Value score;
		if (board.threefold()) {
			score = 0; // Draw by repetition
			// Make sure we're not in check or this is an illegal move
			if (board.side == WHITE) {
				// Black just played a move, so we check that white doesn't control black's king
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[7])).first)
					score = -VALUE_MATE;
			} else {
				if (board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[6])).second)
					score = -VALUE_MATE;
			}
		} else {
			if (i > 3) {
				score = -__recurse(board, depth-reduction(i, depth), -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth-1, -beta, -alpha, -side);
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
			alpha = eval - 200;
			beta = eval + 200;
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
		if (d > 4 && abs(result.second-eval) > 400) // We are probably in a very sharp position, better to not use aspir.
			aspiration_enabled = false;
		eval = result.second;
		best_move = result.first;
		
		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval>0?1:-1) << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
						<< " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		} else {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
						<< " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
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
			alpha = eval - 200;
			beta = eval + 200;
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
		if (d > 4 && abs(result.second-eval) > 400) // We are probably in a very sharp position, better to not use aspir.
			aspiration_enabled = false;
		eval = result.second;
		best_move = result.first;
		
		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score mate " << (VALUE_MATE - abs(eval)) / 2 * (eval>0?1:-1) << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
						<< " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		} else {
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
						<< " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 1000 / TT_SIZE) << " time " << (clock() - start) / CLOCKS_PER_MS << std::endl;
		}

		exit_allowed = true;

		if (abs(eval) >= VALUE_MATE_MAX_PLY) {
			return {best_move, eval};
			// We don't need to search further, we found mate
		}
	}

	return {best_move, eval / CP_SCALE_FACTOR};
}