/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../includes.hpp"
#include "network.hpp"
#include "threats.hpp"
#include "../bitboard.hpp"

#define MAX_THREATS 64

struct AccumulatorManager {
	struct AccumulatorPair {
		int winbucket, binbucket;
		Accumulator w_acc, b_acc;
		bool correct = false;

		void update_add(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
		void update_sub(Square sq, PieceType pt, bool side, int wbucket, int bbucket);
		void update_white_threat_add(int index);
		void update_black_threat_add(int index);
		void update_white_threat_sub(int index);
		void update_black_threat_sub(int index);
	};

	struct PSQTUpdate {
		int w_deltas[4], b_deltas[4];
		int deltas = 0;

		PSQTUpdate() : deltas(0) {}
		PSQTUpdate(int w1, int b1, int w2, int b2) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; deltas = 2; }
		PSQTUpdate(int w1, int b1, int w2, int b2, int w3, int b3) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; deltas = 3; }
		PSQTUpdate(int w1, int b1, int w2, int b2, int w3, int b3, int w4, int b4) { w_deltas[0] = w1; b_deltas[0] = b1; w_deltas[1] = w2; b_deltas[1] = b2; w_deltas[2] = w3; b_deltas[2] = b3; w_deltas[3] = w4; b_deltas[3] = b4; deltas = 4; }
	};

	struct ThreatUpdate {
		int w_adds[MAX_THREATS], w_removes[MAX_THREATS], b_adds[MAX_THREATS], b_removes[MAX_THREATS];
		int widxa = 0, widxr = 0, bidxa = 0, bidxr = 0;

		ThreatUpdate() : widxa(0), widxr(0), bidxa(0), bidxr(0) {}
		void clear() {
			widxa = widxr = bidxa = bidxr = 0;
		}

		void add_white(int index) {
			w_adds[widxa++] = index;
		}
		void add_black(int index) {
			b_adds[bidxa++] = index;
		}
		void remove_white(int index) {
			w_removes[widxr++] = index;
		}
		void remove_black(int index) {
			b_removes[bidxr++] = index;
		}
	};

	struct Cache {
		AccumulatorPair accs[NINPUTS * 2];
		Piece mailboxes[NINPUTS * 2][2][64];

		Cache() {
			std::fill(&mailboxes[0][0][0], &mailboxes[0][0][0] + NINPUTS * 2 * 2 * 64, NO_PIECE);

			for (int i = 0; i < NINPUTS * 2; i++) {
				for (int j = 0; j < L1_SIZE; j++) {
					accs[i].w_acc.val[j] = nnue_network.accumulator_biases[j];
					accs[i].b_acc.val[j] = nnue_network.accumulator_biases[j];
				}
			}
		}
	};

	AccumulatorPair accs[MAX_PLY + 5];
	int idx = 0;
	PSQTUpdate psqtupdates[MAX_PLY + 5]; // Stores the changed indices for each move - updates[i] stores the changes from accs[i-1] to accs[i]
	ThreatUpdate threatupdates[MAX_PLY + 5]; // Stores the changed threat indices for each move - updates[i] stores the changes from accs[i-1] to accs[i]
	Cache finny;

	AccumulatorManager(const AccumulatorManager &) = delete;

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
	 * Records piece and threat changes between pos and pos_after. Refreshes if the
	 * king changes buckets or the threat changes exceed the fixed buffer capacity.
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
