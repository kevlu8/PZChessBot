#include "movepicker.hpp"

std::pair<Move, int> pick_next(pzstd::vector<std::pair<Move, int>> &scores, int &end) {
	if (end == 0) return {NullMove, 0}; // Ran out
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
	std::swap(scores[idx], scores[--end]);
	return {best_move, best_score};
}

MovePicker::MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry) : board(board), ss(ss), ply(ply), main_hist(main_hist) {
	stage = MP_STAGE_TT;
	ttMove = tentry ? tentry->best_move : NullMove;
}

MovePicker::MovePicker(Board &board, History *main_hist, TTable::TTEntry *tentry) : board(board), ss(nullptr), ply(0), main_hist(main_hist) {
	stage = MP_PC_TT;
	ttMove = tentry ? tentry->best_move : NullMove;
}

Move MovePicker::next() {
	if (stage == MP_STAGE_DONE)
		return NullMove;

	if (stage == MP_STAGE_TT) {
		stage = MP_STAGE_GEN;
		if (ttMove != NullMove && board.is_pseudolegal(ttMove))
			return ttMove;
	}

	if (stage == MP_STAGE_GEN) {
		stage = MP_STAGE_GOODNOISY;
		board.legal_moves(moves);

		for (Move move : moves) {
			if (move == ttMove)
				continue;

			int score = 0;

			bool capt = board.is_capture(move);
			bool promo = move.type() == PROMOTION;

			if (capt || promo) {
				if (capt)
					score += MVV[board.mailbox[move.dst()] & 7] +
							 main_hist->capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
				if (promo)
					score += MVV[move.promotion() + KNIGHT] - PawnValue;
				
				noisy_scores.push_back({move, score});
			} else {
				if (qskip) continue;
				score = main_hist->get_history(board, move, ply, ss);
				quiet_scores.push_back({move, score});
			}
		}

		end = noisy_scores.size();
	}

	if (stage == MP_STAGE_GOODNOISY) {
		if (end == 0) {
			stage = MP_STAGE_MOVES;
			end = quiet_scores.size();
			return next();
		}
		while (true) {
			auto [move, score] = pick_next(noisy_scores, end);
			if (move == NullMove) {
				stage = MP_STAGE_MOVES;
				end = quiet_scores.size();
				break;
			}
			Value see = board.see_capture(move);
			if (move.type() == PROMOTION || see >= 0) {
				return move;
			} else {
				bad_noisy.push_back(move);
			}
		}
	}

	if (stage == MP_STAGE_MOVES) {
		if (end == 0) {
			stage = MP_STAGE_BADNOISY;
			end = 0;
			return next();
		}
		auto [move, score] = pick_next(quiet_scores, end);
		if (move != NullMove) return move;
		stage = MP_STAGE_BADNOISY;
		end = 0;
	}

	if (stage == MP_STAGE_BADNOISY) {
		if (end >= bad_noisy.size()) {
			stage = MP_STAGE_DONE;
			return NullMove;
		}
		return bad_noisy[end++];
	}

	if (stage == MP_PC_TT) {
		stage = MP_PC_GEN;
		if (ttMove != NullMove && board.is_pseudolegal(ttMove))
			return ttMove;
	}

	if (stage == MP_PC_GEN) {
		stage = MP_PC_MOVES;
		board.captures(moves);

		for (Move move : moves) {
			if (move == ttMove)
				continue;

			if (move.type() != PROMOTION && board.see_capture(move) <= 105)
				continue; // Skip bad captures

			int score = 0;
			score = MVV[board.mailbox[move.dst()] & 7] +
					main_hist->capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
			noisy_scores.push_back({move, score});
		}

		end = noisy_scores.size();
	}

	if (stage == MP_PC_MOVES) {
		if (end == 0) {
			stage = MP_STAGE_DONE;
			return NullMove;
		}
		auto [move, score] = pick_next(noisy_scores, end);
		if (move != NullMove) return move;
		stage = MP_STAGE_DONE;
		return NullMove;
	}

	if (stage == MP_STAGE_DONE) {
		return NullMove;
	}

	std::cout << "Invalid MovePicker stage: " << stage << std::endl;
	throw std::runtime_error("Il faut que vous mouriez");
}
