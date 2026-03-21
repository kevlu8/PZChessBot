#pragma once

#include "../includes.hpp"
#include "network.hpp"
#include "../bitboard.hpp"

struct AccumulatorManager {
	struct AccumulatorPair {
		Accumulator w_acc, b_acc;
		bool correct = false;

		void update_add(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
		void update_sub(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
	};

	AccumulatorPair accs[MAX_PLY + 5];
	int idx = 0;

	AccumulatorManager(Position &pos) {
		full_refresh(pos, 0);
	}

	AccumulatorPair &current() {
		return accs[idx];
	}

	/**
	 * Recomputes the accumulator at index i from scratch based on the given position.
	 */
	void full_refresh(Position &pos, int index);

	/**
	 * Updates the accumulator stack based on the move. `pos_after` is only used if the king crosses
	 * a boundary, in which case we do a full refresh. Otherwise, we just do an incremental update.
	 */
	void make_move(Position &pos, Move move, Position &pos_after);

	/**
	 * Reverts previously made move, restoring the previous accumulator. There are no safeguards to
	 * prevent popping past the beginning.
	 */
	void pop_move() {
		idx--;
	}
};