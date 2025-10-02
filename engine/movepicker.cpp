#include "movepicker.hpp"

MovePicker::MovePicker(Board &board, SSEntry *ss, int ply, History *main_hist, TTable::TTEntry *tentry) : board(board), ss(ss), ply(ply), main_hist(main_hist) {
	stage = MP_STAGE_TT;
	ttMove = tentry ? tentry->best_move : NullMove;
}

Move MovePicker::next() {
	if (stage == MP_STAGE_DONE)
		return NullMove;

	if (stage == MP_STAGE_TT) {
		stage = MP_STAGE_KILLER1;
		if (ttMove != NullMove && board.is_pseudolegal(ttMove))
			return ttMove;
	}

	if (stage == MP_STAGE_KILLER1) {
		stage = MP_STAGE_KILLER2;
		if (ss->killer[0] != NullMove && ss->killer[0] != ttMove && board.is_pseudolegal(ss->killer[0]))
			return ss->killer[0];
	}

	if (stage == MP_STAGE_KILLER2) {
		stage = MP_STAGE_GEN;
		if (ss->killer[1] != NullMove && ss->killer[1] != ttMove && ss->killer[1] != ss->killer[0] && board.is_pseudolegal(ss->killer[1]))
			return ss->killer[1];
	}

	if (stage == MP_STAGE_GEN) {
		stage = MP_STAGE_SCORE;
		board.legal_moves(moves);
		end = moves.size();
	}

	if (stage == MP_STAGE_SCORE) {
		stage = MP_STAGE_MOVES;

		const int CAPTURE_PROMO_BASE = 1000000;
		const int QUIET_BASE = 0;

		for (Move move : moves) {
			if (move == ttMove || move == ss->killer[0] || move == ss->killer[1])
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
			} else {
				score = QUIET_BASE + main_hist->get_history(board, move, ply, ss);
			}
			scores.push_back(std::make_pair(move, score));
		}
	}

	if (stage == MP_STAGE_MOVES) {
		if (end == 0) {
			stage = MP_STAGE_DONE;
			return NullMove;
		}

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
		std::swap(scores[idx], scores[end - 1]);
		end--;
		return best_move;
	}

	if (stage == MP_STAGE_DONE) {
		return NullMove;
	}

	throw std::runtime_error("Invalid MovePicker stage");
}
