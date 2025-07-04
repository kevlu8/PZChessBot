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

Move killer[2][MAX_PLY];
Value history[2][64][64];

void clear_tables() {
	for (int i = 0; i < MAX_PLY; i++) {
		killer[0][i] = killer[1][i] = NullMove;
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

pzstd::vector<std::pair<Move, Value>> assign_values(pzstd::vector<Move> &moves, Board &board, int ply) {
	pzstd::vector<std::pair<Move, Value>> scores;
	for (Move &m : moves) {
		if (is_capture(m, board)) {
			scores.push_back({ m, MVV_LVA[board.mailbox[m.src()] & 7][board.mailbox[m.dst()] & 7] + MVV_LVA_C });
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

std::pair<Move, Value> negamax(Board &board, int depth, int side, int ply = 0, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE) {
	nodes++;

	if (!(nodes & 4095)) {
		int elapsed_time = (clock() - start_time) / CLOCKS_PER_MS;
		if (elapsed_time >= max_time) {
			stop_search = true;
			return { NullMove, -VALUE_INFINITE };
		}
	}

	bool wking_control = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second;
	bool bking_control = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first;

	bool in_check = board.side == WHITE ? wking_control : bking_control;
	bool opp_in_check = board.side == WHITE ? bking_control : wking_control;

	if (opp_in_check) // If it's our turn and our opponent is in check, we win
		return { NullMove, VALUE_MATE };

	if (board.threefold() || board.halfmove >= 100) { // If the position is threefold or 50-move rule applies, draw
		return { NullMove, 0 };
	}

	if (depth <= 0) {
		return { NullMove, eval(board) * side };
	}

	Move best_move = NullMove;
	Value best = -VALUE_INFINITE;
	
	pzstd::vector<Move> moves;
	board.legal_moves(moves);

	pzstd::vector<std::pair<Move, Value>> scores = assign_values(moves, board, ply);

	pzstd::vector<Move> captures, quiets;
	
	Move m = NullMove;
	while ((m = next_move(scores)) != NullMove) {
		board.make_move(m);
		Value score = -negamax(board, depth - 1, -side, ply + 1, -beta, -alpha).second;
		board.unmake_move();

		if (score >= VALUE_MATE_MAX_PLY) score--; // Adjust for mate in n ply
		if (score <= -VALUE_MATE_MAX_PLY) score++;

		if (score > best) {
			best_move = m;
			best = score;
			if (score > alpha) {
				alpha = score;
			}
		}

		if (score >= beta) {
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
			return { best_move, beta }; // Beta cutoff
		}

		if (stop_search) return { NullMove, -VALUE_INFINITE };

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

	return { best_move, best };
}

std::pair<Move, Value> search(Board &board, int64_t time) {
	clear_tables();

	Move cur_move = NullMove;
	Value cur_eval = -VALUE_INFINITE;

	start_time = clock();
	max_time = time;
	stop_search = false;

	for (int depth = 1; depth <= MAX_PLY; depth++) {
		nodes = 0;
		
		auto res = negamax(board, depth, board.side == WHITE ? 1 : -1);
		Move best_move = res.first;
		Value best = res.second;

		int time_elapsed = (clock() - start_time) / CLOCKS_PER_MS;
		if (time_elapsed == 0) time_elapsed = 1; // Avoid division by zero (plus, who cares about 1 ms?)
		if (time_elapsed >= time) {
			break;
		}

		if (best_move != NullMove) {
			cur_move = best_move;
			cur_eval = best;

			std::cout << "info depth " << depth << " score " << score_to_string(best) << " pv " << best_move.to_string() << " nodes " << nodes << " time " << time_elapsed << " nps " << nodes * 1000 / time_elapsed << std::endl;
		}
	}

	return { cur_move, cur_eval };
}

std::pair<Move, Value> search_depth(Board &board, int depth) {
	clear_tables();

	Move cur_move = NullMove;
	Value cur_eval = -VALUE_INFINITE;

	start_time = clock();
	max_time = 1e18; // Inf until we hit depth

	nodes = 0;
	auto res = negamax(board, depth, board.side == WHITE ? 1 : -1);
	Move best_move = res.first;
	Value best = res.second;

	int time_elapsed = (clock() - start_time) / CLOCKS_PER_MS;
	if (time_elapsed == 0) time_elapsed = 1; // Avoid division by zero

	if (best_move != NullMove) {
		cur_move = best_move;
		cur_eval = best;

		std::cout << "info depth " << depth << " score " << score_to_string(best) << " pv " << best_move.to_string() << " nodes " << nodes << " time " << time_elapsed << " nps " << nodes * 1000 / time_elapsed << std::endl;
	}

	return { cur_move, cur_eval };
}
