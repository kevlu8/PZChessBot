#include "search.hpp"

uint64_t nodes = 0;
clock_t start_time = 0;
uint64_t max_time = 0;
bool stop_search = false;

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

Move pvtable[MAX_PLY][MAX_PLY];
int pvsz[MAX_PLY];

std::string format_pv() {
	std::string result;
	for (int i = 0; i < MAX_PLY && pvtable[0][i] != NullMove; i++) {
		result += pvtable[0][i].to_string() + " ";
	}
	if (!result.empty()) result.pop_back(); // remove trailing space
	return result;
}

Move killer[2][MAX_PLY];
Value history[2][64][64];

void clear_tables() {
	for (int i = 0; i < MAX_PLY; i++) {
		killer[0][i] = killer[1][i] = NullMove;
		pvsz[i] = 0;
	}
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			history[0][i][j] = history[1][i][j] = 0;
		}
	}
}

void update_history(int side, Square from, Square to, int bonus) {
	Value cbonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
	history[side][from][to] += cbonus - history[side][from][to] * abs(cbonus) / MAX_HISTORY;
}

pzstd::vector<std::pair<Move, Value>> assign_values(pzstd::vector<Move> &moves, Board &board, int ply, Move hash_move = NullMove) {
	pzstd::vector<std::pair<Move, Value>> scores;
	for (Move &m : moves) {
		if (m == hash_move) {
			scores.push_back({ m, VALUE_INFINITE }); // Prefer the hash move
			continue;
		}
		if (is_capture(m, board)) {
			scores.push_back({ m, MVV_LVA[board.mailbox[m.dst()] & 7][board.mailbox[m.src()] & 7] + MVV_LVA_C });
		} else {
			Value score = 0;
			if (m == killer[0][ply]) score += 1500;
			else if (m == killer[1][ply]) score += 1000;
			score += history[board.side][m.src()][m.dst()];
			scores.push_back({ m, score });
		}
	}
	return scores;
}

pzstd::vector<std::pair<Move, Value>> assign_values_qs(pzstd::vector<Move> &moves, Board &board) {
	pzstd::vector<std::pair<Move, Value>> scores;
	for (Move &m : moves) {
		if (is_capture(m, board)) {
			scores.push_back({ m, MVV_LVA[board.mailbox[m.dst()] & 7][board.mailbox[m.src()] & 7] + MVV_LVA_C });
		}
	}
	return scores;
}

Move next_move(pzstd::vector<std::pair<Move, Value>> &moves) {
	int idx = 0;
	Move best = NullMove;
	Value score = -VALUE_INFINITE;
	for (int i = 0; i < moves.size(); i++) {
		if (moves[i].second > score) {
			score = moves[i].second;
			best = moves[i].first;
			idx = i;
		}
	}
	moves[idx] = { NullMove, -VALUE_INFINITE }; // Remove the best move from the list
	return best;
}

Value quiesce(Board &board, int side, Value alpha, Value beta, int ply = 0) {
	Value stand_pat = eval(board) * side;
	if (stand_pat >= beta) return stand_pat;
	if (stand_pat > alpha) alpha = stand_pat;

	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	pzstd::vector<std::pair<Move, Value>> scores = assign_values_qs(moves, board);

	Move m = NullMove;
	while ((m = next_move(scores)) != NullMove) {
		board.make_move(m);
		Value score = -quiesce(board, -side, -beta, -alpha, ply + 1);
		board.unmake_move();

		if (score > stand_pat) {
			stand_pat = score;
			if (score > alpha) {
				alpha = score;
			}
		}
		if (score >= beta) return stand_pat;
	}

	return stand_pat;
}

Value negamax(Board &board, int depth, int side, int ply = 0, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE) {
	nodes++;

	pvsz[ply] = 0;

	if (!(nodes & 4095)) {
		int elapsed_time = (clock() - start_time) / CLOCKS_PER_MS;
		if (elapsed_time >= max_time) {
			stop_search = true;
			return 0;
		}
	}

	bool wking_control = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second;
	bool bking_control = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first;

	bool in_check = board.side == WHITE ? wking_control : bking_control;
	bool opp_in_check = board.side == WHITE ? bking_control : wking_control;

	if (opp_in_check) // If it's our turn and our opponent is in check, we win
		return VALUE_MATE;

	if (board.threefold() || board.halfmove >= 100) { // If the position is threefold or 50-move rule applies, draw
		return 0;
	}

	// Probe for TTable cutoffs
	TTable::TTEntry *entry = board.ttable.probe(board.zobrist);
	Move hash_move = NullMove;
	if (entry) {
		hash_move = entry->best_move;
		if (entry->depth >= depth) {
			if (entry->flags == EXACT) {
				return entry->eval;
			} else if (entry->flags == LOWER_BOUND && entry->eval >= beta) {
				return entry->eval;
			} else if (entry->flags == UPPER_BOUND && entry->eval <= alpha) {
				return entry->eval;
			}
		}
	}

	if (depth <= 0) {
		return quiesce(board, side, alpha, beta, ply);
	}

	Value og_alpha = alpha; // Store original alpha for TT

	Move best_move = NullMove;
	Value best = -VALUE_INFINITE;
	
	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	pzstd::vector<std::pair<Move, Value>> scores = assign_values(moves, board, ply, hash_move);

	pzstd::vector<Move> captures, quiets;
	
	Move m = NullMove;
	while ((m = next_move(scores)) != NullMove) {
		board.make_move(m);
		Value score = -negamax(board, depth - 1, -side, ply + 1, -beta, -alpha);
		board.unmake_move();

		if (score >= VALUE_MATE_MAX_PLY) score--; // Adjust for mate in n ply
		if (score <= -VALUE_MATE_MAX_PLY) score++;

		if (score > best) {
			best_move = m;
			best = score;
			if (score > alpha) {
				alpha = score;
				if (score < beta) {
					pvtable[ply][0] = m; // Store the principal variation
					pvsz[ply] = pvsz[ply+1] + 1;
					for (int i = 0; i < pvsz[ply+1]; i++) {
						pvtable[ply][i+1] = pvtable[ply+1][i];
					}
				}
			}
		}

		if (score >= beta) {
			board.ttable.store(board.zobrist, score, depth, LOWER_BOUND, best_move, board.halfmove);
			if (!is_capture(m, board)) {
				if (m != killer[0][ply] && m != killer[1][ply]) {
					// Store the killer move
					killer[1][ply] = killer[0][ply];
					killer[0][ply] = m;
				}
				const Value bonus = depth * depth;
				update_history(board.side, m.src(), m.dst(), bonus);
				for (Move &q : quiets) {
					update_history(board.side, q.src(), q.dst(), -bonus);
				}
			}
			return best;
		}

		if (stop_search) return -VALUE_INFINITE;

		if (is_capture(m, board)) {
			captures.push_back(m);
		} else if (m.type() != PROMOTION) {
			quiets.push_back(m);
		}
	}

	if (abs(best) == VALUE_MATE - 1) {
		// Engine thinks we are currently one move from king being taken
		// However, if we aren't in check, this is a stalemate
		if (!in_check) {
			best = 0; // Stalemate
			best_move = NullMove; // No best move in stalemate
		}
	}

	if (best <= og_alpha) {
		board.ttable.store(board.zobrist, best, depth, UPPER_BOUND, best_move, board.halfmove);
	} else {
		board.ttable.store(board.zobrist, best, depth, EXACT, best_move, board.halfmove);
	}

	return best;
}

std::pair<Move, Value> search(Board &board, int64_t time, int mx_depth, uint64_t nodes_limit) {
	clear_tables();

	Move cur_move = NullMove;
	Value cur_eval = -VALUE_INFINITE;

	start_time = clock();
	max_time = time;
	stop_search = false;

	for (int depth = 1; depth <= mx_depth; depth++) {
		nodes = 0;

		Value lo = -VALUE_INFINITE, hi = VALUE_INFINITE;
		int lwindow_sz = ASPIRATION_SIZE, hwindow_sz = ASPIRATION_SIZE;
		if (cur_eval != -VALUE_INFINITE) {
			// Aspiration windows 
			lo = cur_eval - lwindow_sz;
			hi = cur_eval + hwindow_sz;
		}
		auto res = negamax(board, depth, board.side == WHITE ? 1 : -1, 0, lo, hi);
		while (!(lo < res && res < hi)) {
			// If the result is outside the aspiration window, we need to widen it
			// Luckily this won't happen when we are at infinite bounds therefore we don't need to handle that
			if (res <= lo) {
				// Failed low, expand lower bound
				lwindow_sz *= 4;
			} else if (res >= hi) {
				// Failed high, expand upper bound
				hwindow_sz *= 4;
			}
			lo = cur_eval - lwindow_sz;
			hi = cur_eval + hwindow_sz;
			if (hwindow_sz + lwindow_sz > VALUE_INFINITE / 8) {
				// If the window is too large, we just use infinite bounds
				lo = -VALUE_INFINITE;
				hi = VALUE_INFINITE;
			}
			res = negamax(board, depth, board.side == WHITE ? 1 : -1, 0, lo, hi);
			if (stop_search) {
				// We didn't have enough time to finish this depth
				break;
			}
		}

		Move best_move = pvtable[0][0];
		Value best = res;

		double time_elapsed = (clock() - start_time) / (double)CLOCKS_PER_MS;
		if (time_elapsed == 0) time_elapsed = 0.0001; // Avoid division by zero
		if (time_elapsed >= time || stop_search || nodes >= nodes_limit) {
			break;
		}

		if (best_move != NullMove) {
			cur_move = best_move;
			cur_eval = best;

			std::cout << "info depth " << depth << " score " << score_to_string(best) << " pv " << format_pv() << " nodes " << nodes << " time " << int(time_elapsed) << " nps " << int(nodes * 1000 / time_elapsed) << std::endl;
		}
	}

	return { cur_move, cur_eval };
}
