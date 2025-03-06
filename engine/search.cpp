#include "search.hpp"

#define MOVENUM(x) ((((#x)[1]-'1') << 12) | (((#x)[0]-'a') << 8) | (((#x)[3]-'1') << 4) | ((#x)[2]-'a'))

uint64_t nodes = 0, tbhits = 0, nexp = 0;
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
	std::vector<Move> moves;
	board.legal_moves(moves);
	uint64_t cnt = 0;
	for (Move &move : moves) {
		board.make_move(move);
		cnt += perft(board, depth - 1);
		board.unmake_move();
	}
	return cnt;
}

Value quiesce(Board &board, Value alpha, Value beta, int side) {
	nodes++;
	return eval(board) * side; // temporary
	// Value stand_pat = eval(board) * side;

	// if (stand_pat == VALUE_MATE || stand_pat == -VALUE_MATE)
	// 	return stand_pat;

	// if (stand_pat >= beta)
	// 	return stand_pat;
	// if (stand_pat > alpha)
	// 	alpha = stand_pat;

	// std::vector<Move> moves;
	// board.legal_moves(moves);

	// Value best = stand_pat;

	// for (int i = 0; i < moves.size(); i++) {
	// 	Move &move = moves[i];
	// 	if (board.piece_boards[OPPOCC(board.side)] & square_bits(move.dst())) {
	// 		board.make_move(move);
	// 		Value score = -quiesce(board, -beta, -alpha, -side);
	// 		board.unmake_move();

	// 		if (score > best) {
	// 			if (score > alpha)
	// 				alpha = score;
	// 			best = score;
	// 		}
	// 		if (score >= beta) {
	// 			return best;
	// 		}
	// 	} else {
	// 		board.make_move(move);
	// 		// check if it's a check
	// 		std::pair<int, int> check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(board.side)]));
	// 		if (board.side == WHITE && check.second || board.side == BLACK && check.first) {
	// 			Value score = -quiesce(board, -beta, -alpha, -side);
	// 			board.unmake_move();

	// 			if (score > best) {
	// 				if (score > alpha)
	// 					alpha = score;
	// 				best = score;
	// 			}
	// 			if (score >= beta) {
	// 				return best;
	// 			}
	// 		} else board.unmake_move();
	// 	}
	// }

	// return best;
}

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	if (depth <= 0) {
		return quiesce(board, alpha, beta, side);
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

	std::vector<Move> moves;
	board.legal_moves(moves);

	// Move ordering (experimental)
	if (depth > 2) {
		std::vector<std::pair<Move, Value>> scores;
		for (Move &move : moves) {
			board.make_move(move);
			Value score = 0;
			TTable::TTEntry *entry = board.ttable.probe(board.zobrist);
			if (entry->flags != INVALID) {
				score = entry->eval * side;
				tbhits++;
			} else score = eval(board) * side;
			board.unmake_move();
			scores.push_back({move, score});
		}
		std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
			return a.second > b.second;
		});
	}

	Move best_move = NullMove;

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);

		Value score = -__recurse(board, depth - 1, -beta, -alpha, -side);
		// Value score = -__recurse(board, i > 15 ? depth - 2 : depth - 1, -beta, -alpha, -side);

		if (abs(score) >= VALUE_MATE_MAX_PLY)
			score = score - (uint16_t(score >> 15) << 1) - 1;

		board.ttable.store(board.zobrist, score, depth, EXACT, move, board.halfmove);
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

	board.ttable.store(board.zobrist, best, depth, EXACT, best_move, board.halfmove);

	return best;
}

std::pair<Move, Value> __search(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	Move best_move = NullMove;
	Value best_score = -VALUE_INFINITE;

	std::vector<Move> moves;
	board.legal_moves(moves);

	if (depth > 2) {
		std::vector<std::pair<Move, Value>> scores;
		for (Move &move : moves) {
			board.make_move(move);
			Value score = 0;
			TTable::TTEntry *entry = board.ttable.probe(board.zobrist);
			if (entry->flags != INVALID) {
				score = entry->eval * side;
				tbhits++;
			} else score = eval(board) * side;
			board.unmake_move();
			scores.push_back({move, score});
		}
		std::stable_sort(scores.begin(), scores.end(), [&](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) {
			return a.second > b.second;
		});
	}

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);
		Value score;
		if (board.dtable.occ(board.zobrist) >= 2) {
			score = 0; // Draw by repetition
		} else {
			if (i) {
				score = -__recurse(board, depth - 1, -alpha - 1, -alpha, -side);
				if (score > alpha && score < beta) {
					score = -__recurse(board, depth - 1, -beta, -alpha, -side);
				}
			} else {
				score = -__recurse(board, depth - 1, -beta, -alpha, -side);
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
			break;
		}

		if (nodes > nexp && exit_allowed) {
			early_exit = true;
			break;
		}
	}

	board.ttable.store(board.zobrist, best_score, depth, EXACT, best_move, board.halfmove);

	return {best_move, best_score};
}

std::pair<Move, Value> search(Board &board, int depth) {
	nodes = tbhits = 0;
	early_exit = exit_allowed = false;
	clock_t start = clock();

	Move best_move = NullMove;
	Value eval = -VALUE_INFINITE;
	if (depth == -1 || depth >= 50) { // Do iterative deepening
		if (depth == -1) nexp = 50'000'000;
		else nexp = depth;
		for (int d = 1; d <= 20; d++) {
			Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;
			if (eval != -VALUE_INFINITE) {
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
			eval = result.second;
			best_move = result.first;
			
			std::cout << "info depth " << d << " score cp " << eval << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
					  << " tbhits " << tbhits << " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 100 / TT_SIZE) << std::endl;
			// I know tbhits isn't correct here, I'm just using it to show number of TT hits

			exit_allowed = true;

			if (eval >= VALUE_MATE_MAX_PLY) {
				// we found mate, no need to search further
				break;
			}

			if (nodes > nexp)
				break;
		}
	} else {
		auto result = __search(board, depth, -VALUE_INFINITE, VALUE_INFINITE, board.side ? -1 : 1);
		best_move = result.first;
		eval = result.second;
		std::cout << "info depth " << depth << " score cp " << eval << " nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC))
				  << " tbhits " << tbhits << " pv " << best_move.to_string() << " hashfull " << (board.ttable.size() * 100 / TT_SIZE) << std::endl;
	}

	return {best_move, eval};
}
