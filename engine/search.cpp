#include "search.hpp"

#define MOVENUM(x) ((((#x)[1]-'1') << 12) | (((#x)[0]-'a') << 8) | (((#x)[3]-'1') << 4) | ((#x)[2]-'a'))

uint64_t nodes = 0, tbhits = 0, nexp = 0;
int seldepth = 0;
bool early_exit = false, exit_allowed = false;

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
	// if (i > 31) return 3;
	// if (i > 15) return 2;
	return 1;
}

Value quiesce(Board &board, Value alpha, Value beta, int side, int depth) {
	nodes++;
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

	Value best = stand_pat;

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		if (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst())) {
			board.make_move(move);
			Value score = -quiesce(board, -beta, -alpha, -side, depth+1);
			board.unmake_move();

			if (score > best) {
				if (score > alpha)
					alpha = score;
				best = score;
			}
			if (score >= beta) {
				return best;
			}
		}
	}

	return best;
}

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
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

	Value best = -VALUE_INFINITE;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	// Move ordering (experimental)
	pzstd::vector<std::pair<Move, Value>> scores;
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, alpha, beta, depth);
	Move entry = tentry ? tentry->best_move : NullMove;
	bool entry_is_legal = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
		tbhits++;
	} else entry_is_legal = true; // Don't skip the first move if we never added it
	for (Move &move : moves) {
		if (move == entry) {
			entry_is_legal = true;
			continue; // Don't add the TT move again
		}
		board.make_move(move);
		Value score = 0;
		score = eval(board) * side;
		board.unmake_move();
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
		return a.second > b.second;
	});
	for (int i = 0; i < scores.size(); i++) moves[i] = scores[i].first;

	Move best_move = NullMove;

	bool first = true;

	for (int i = !entry_is_legal; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);

		Value score;
		if (!first) {
			score = -__recurse(board, depth-reduction(i, depth), -alpha - 1, -alpha, -side);
			if (score > alpha && score < beta) {
				score = -__recurse(board, depth-reduction(i, depth), -beta, -alpha, -side);
			}
		} else {
			score = -__recurse(board, depth - 1, -beta, -alpha, -side);
			first = false;
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

		if (nodes > nexp && exit_allowed) {
			early_exit = true;
			break;
		}
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
	TTable::TTEntry *tentry = board.ttable.probe(board.zobrist, alpha, beta, depth);
	Move entry = tentry ? tentry->best_move : NullMove;
	bool entry_is_legal = false;
	if (entry != NullMove) {
		scores.push_back({entry, VALUE_INFINITE}); // Make the TT move first
		tbhits++;
	} else entry_is_legal = true; // Don't skip the first move if we never added it
	for (Move &move : moves) {
		if (move == entry) {
			entry_is_legal = true;
			continue; // Don't add the TT move again
		}
		board.make_move(move);
		Value score = 0;
		score = eval(board) * side;
		board.unmake_move();
		scores.push_back({move, score});
	}
	std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
		return a.second > b.second;
	});
	for (int i = 0; i < scores.size(); i++) moves[i] = scores[i].first;

	bool first = true;

	for (int i = !entry_is_legal; i < moves.size(); i++) { // Skip the TT move if it's not legal
		Move &move = moves[i];
		board.make_move(move);
		Value score;
		if (board.dtable.occ(board.zobrist) >= 2) {
			score = 0; // Draw by repetition
		} else {
			if (!first) {
				score = -__recurse(board, depth-reduction(i, depth), -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth-reduction(i, depth), -beta, -alpha, -side);
				}
			} else {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side);
				first = false;
			}
		}
		board.ttable.store(board.zobrist, score, depth, EXACT, move, board.halfmove);
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

		if (nodes > nexp && exit_allowed) {
			early_exit = true;
			break;
		}
	}

	if (best_score <= alpha) {
		board.ttable.store(board.zobrist, alpha, depth, UPPER_BOUND, best_move, board.halfmove);
	} else {
		board.ttable.store(board.zobrist, best_score, depth, EXACT, best_move, board.halfmove);
	}

	return {best_move, best_score};
}

std::pair<Move, Value> search(Board &board, int64_t depth) {
	nodes = tbhits = seldepth = 0;
	early_exit = exit_allowed = false;
	clock_t start = clock();

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	if (depth == -1 || depth >= 50) { // Do iterative deepening
		if (depth == -1) nexp = 30'000'000;
		else nexp = depth;
		bool aspiration_enabled = true;
		for (int d = 1; d <= 20; d++) {
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
			if (early_exit) {
				break;
			}
			if (d > 4 && abs(result.second-eval) > 400) // We are probably in a very sharp position, better to not use aspir.
				aspiration_enabled = false;
			eval = result.second;
			best_move = result.first;
			
			std::cout << "info depth " << d << " seldepth " << d + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
					  << " tbhits " << tbhits << " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 100 / TT_SIZE) << std::endl;
			// I know tbhits isn't correct here, I'm just using it to show number of TT hits

			exit_allowed = true;

			if (nodes > nexp)
				break;
		}
	} else {
		auto result = __search(board, depth, -VALUE_INFINITE, VALUE_INFINITE, board.side ? -1 : 1);
		best_move = result.first;
		eval = result.second;
		std::cout << "info depth " << depth << " seldepth " << depth + seldepth << " score cp " << eval / CP_SCALE_FACTOR << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
				  << " tbhits " << tbhits << " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 100 / TT_SIZE) << std::endl;
	}

	return {best_move, eval};
}
