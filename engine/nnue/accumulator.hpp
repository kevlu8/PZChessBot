#pragma once

#include "../includes.hpp"
#include "network.hpp"
#include "../bitboard.hpp"

#define NEGATE 30000

struct AccumulatorManager {
	struct AccumulatorPair {
		int winbucket, binbucket;
		Accumulator w_acc, b_acc;
		bool correct = false;

		void update_add(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
		void update_sub(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
	};

	struct Update {
		int w_deltas[4], b_deltas[4];
		int deltas = 0;

		Update() : deltas(0) {}
		Update(int w1, int b1) { w_deltas[0] = w1; b_deltas[0] = b1; deltas = 1; }
		Update(int w1, int b1, int w2, int b2) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; deltas = 2; }
		Update(int w1, int b1, int w2, int b2, int w3, int b3) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; deltas = 3; }
		Update(int w1, int b1, int w2, int b2, int w3, int b3, int w4, int b4) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; w_deltas[3] = w4; b_deltas[3] = b4; deltas = 4; }
	};

	struct Cache {
		AccumulatorPair accs[NINPUTS * 2];
		Piece mailboxes[NINPUTS * 2][2][64];

		Cache() {
			std::fill(&mailboxes[0][0][0], &mailboxes[0][0][0] + NINPUTS * 2 * 2 * 64, NO_PIECE);

			for (int i = 0; i < NINPUTS * 2; i++) {
				for (int j = 0; j < HL_SIZE; j++) {
					accs[i].w_acc.val[j] = nnue_network.accumulator_biases[j];
					accs[i].b_acc.val[j] = nnue_network.accumulator_biases[j];
				}
			}
		}
	};

	AccumulatorPair accs[MAX_PLY + 5];
	int idx = 0;
	Update updates[MAX_PLY + 5]; // Stores the changed indices for each move - updates[i] stores the changes from accs[i-1] to accs[i]
	Cache finny;

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
	 * Recomputes the accumulator at index i using finny tables
	 */
	void refresh_finny(Position &pos, int index);

	/**
	 * Updates the accumulator stack
	 */
	void apply_lazy(Position &pos);

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
		accs[idx].correct = false;
		idx--;
	}
};