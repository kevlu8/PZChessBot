#include "movepicker.hpp"

std::pair<Move, int> next_score(pzstd::vector<std::pair<Move, int>> &scores, int &end) {
	if (end == 0) return { NullMove, 0 }; // Ran out
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
	return { best_move, best_score };
}

MovePicker::MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry) : board(board), ss(ss), ply(ply), main_hist(main_hist) {
	stage = MP_STAGE_TT;
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

		const int CAPTURE_PROMO_BASE = 1000000;
		const int QUIET_BASE = 0;

		for (Move move : moves) {
			if (move == ttMove)
				continue;

			int score = 0;

			bool capt = board.is_capture(move);
			bool promo = move.type() == PROMOTION;

			if (capt || promo) {
				score = CAPTURE_PROMO_BASE;
				if (capt)
					score += PieceValue[board.mailbox[move.dst()] & 7] +
							 main_hist->capthist[board.mailbox[move.src()] & 7][board.mailbox[move.dst()] & 7][move.dst()];
				if (promo)
					score += PieceValue[move.promotion() + KNIGHT] - PawnValue;
				
				scores_noisy.push_back({move, score});
			} else {
				if (qskip) continue;
				score = QUIET_BASE + main_hist->get_history(board, move, ply, ss);
				if (move == ss->killer) score += 1457;
				scores_quiet.push_back({move, score});
			}
		}

		end = scores_noisy.size();
	}

	if (stage == MP_STAGE_GOODNOISY) {
		if (end == 0) {
			stage = MP_STAGE_QUIET;
			end = scores_quiet.size();
		} else {
			while (true) {
				auto [move, score] = next_score(scores_noisy, end);
				if (move == NullMove) break;

				Value see = board.see_capture(move);
				if (see < -100) {
					scores_badnoisy.push_back({move, score});
				} else {
					return move;
				}
			}
			stage = MP_STAGE_QUIET;
			end = scores_quiet.size();
		}
	}

	if (stage == MP_STAGE_QUIET) {
		if (end == 0 || qskip) {
			stage = MP_STAGE_BADNOISY;
			end = scores_badnoisy.size();
		} else {
			auto [move, score] = next_score(scores_quiet, end);
			if (move == NullMove) {
				stage = MP_STAGE_BADNOISY;
				end = scores_badnoisy.size();
			} else {
				return move;
			}
		}
	}

	if (stage == MP_STAGE_BADNOISY) {
		if (end == 0) {
			stage = MP_STAGE_DONE;
			return NullMove;
		}
		auto [move, score] = next_score(scores_badnoisy, end);
		if (move == NullMove) {
			stage = MP_STAGE_DONE;
			return NullMove;
		} else {
			return move;
		}
	}

	if (stage == MP_STAGE_DONE) {
		return NullMove;
	}

	std::cout << "Invalid MovePicker stage: " << stage << std::endl;
	throw std::runtime_error("Il faut que vous mouriez");
}
