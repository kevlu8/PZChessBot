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

#include "accumulator.hpp"

void AccumulatorManager::AccumulatorPair::update_add(Square sq, PieceType pt, bool side, int wbucket, int bbucket) {
	uint16_t w_index = calculate_index(sq, pt, side, 0, wbucket);
	uint16_t b_index = calculate_index(sq, pt, side, 1, bbucket);
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] += nnue_network.accumulator_weights[w_index][i];
		b_acc.val[i] += nnue_network.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_sub(Square sq, PieceType pt, bool side, int wbucket, int bbucket) {
	uint16_t w_index = calculate_index(sq, pt, side, 0, wbucket);
	uint16_t b_index = calculate_index(sq, pt, side, 1, bbucket);
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] -= nnue_network.accumulator_weights[w_index][i];
		b_acc.val[i] -= nnue_network.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::full_refresh(Position &pos, int index) {
	// Init the first accumulator so we have a basepoint
	for (int i = 0; i < L1_SIZE; i++) {
		accs[index].w_acc.val[i] = nnue_network.accumulator_biases[i];
		accs[index].b_acc.val[i] = nnue_network.accumulator_biases[i];
	}

	Square wkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];
	accs[index].winbucket = winbucket;
	accs[index].binbucket = binbucket;

	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);

		if (piece != NO_PIECE) {
			// Add to accumulator
			accs[index].update_add((Square)i, pt, side, winbucket, binbucket);
		}
	}

	accs[index].correct = true;
}

void AccumulatorManager::refresh_finny(Position &pos, int index) {
	int winbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])];
	int binbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]) ^ 56];
	accs[index].winbucket = winbucket;
	accs[index].binbucket = binbucket;

	Accumulator &f_w_acc = finny.accs[winbucket].w_acc;
	Accumulator &f_b_acc = finny.accs[binbucket].b_acc;
	Accumulator &w_acc = accs[index].w_acc;
	Accumulator &b_acc = accs[index].b_acc;

	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] = f_w_acc.val[i];
		b_acc.val[i] = f_b_acc.val[i];
	}

	for (int i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);
		
		Piece prev_w_piece = finny.mailboxes[winbucket][WHITE][i];
		bool prev_w_side = prev_w_piece >> 3;
		PieceType prev_w_pt = PieceType(prev_w_piece & 7);

		if (piece != prev_w_piece) {
			if (piece != NO_PIECE) {
				// Add to accumulator
				int index = calculate_index((Square)i, pt, side, 0, winbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					w_acc.val[k] += nnue_network.accumulator_weights[index][k];
				}
			}

			if (prev_w_piece != NO_PIECE) {
				// Remove from accumulator
				int index = calculate_index((Square)i, prev_w_pt, prev_w_side, 0, winbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					w_acc.val[k] -= nnue_network.accumulator_weights[index][k];
				}
			}
		}

		Piece prev_b_piece = finny.mailboxes[binbucket][BLACK][i];
		bool prev_b_side = prev_b_piece >> 3;
		PieceType prev_b_pt = PieceType(prev_b_piece & 7);

		if (piece != prev_b_piece) {
			if (piece != NO_PIECE) {
				// Add to accumulator
				int index = calculate_index((Square)i, pt, side, 1, binbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					b_acc.val[k] += nnue_network.accumulator_weights[index][k];
				}
			}

			if (prev_b_piece != NO_PIECE) {
				// Remove from accumulator
				int index = calculate_index((Square)i, prev_b_pt, prev_b_side, 1, binbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					b_acc.val[k] -= nnue_network.accumulator_weights[index][k];
				}
			}
		}
	}

	// Update finny tables
	for (int i = 0; i < L1_SIZE; i++) {
		f_w_acc.val[i] = w_acc.val[i];
		f_b_acc.val[i] = b_acc.val[i];
	}

	for (int i = 0; i < 64; i++) {
		finny.mailboxes[winbucket][WHITE][i] = pos.mailbox[i];
		finny.mailboxes[binbucket][BLACK][i] = pos.mailbox[i];
	}

	accs[index].correct = true;
}

void AccumulatorManager::apply_lazy(Position &pos) {
	if (current().correct) return; // No updates needed
	int index = idx, last_same_bucket = idx;
	bool good_found = false;
	while (true) {
		index--;

		if (accs[index].winbucket != current().winbucket || accs[index].binbucket != current().binbucket) {
			// A bucket change occurred meaning we can't do any incremental updates past this point
			break;
		}

		last_same_bucket = index;

		if (accs[index].correct) {
			// Found a basepoint we can do incremental off of
			// Note that it is implied that the buckets are the same and have not changed
			good_found = true;
			break;
		}
	}

	if (!good_found) {
		// :(
		refresh_finny(pos, idx);
		return;
	}

	for (int i = index + 1; i <= idx; i++) {
		auto &u = updates[i];
		if (u.deltas == 2) {
			// -+
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nnue_network.accumulator_weights[u.w_deltas[0]][k] + nnue_network.accumulator_weights[u.w_deltas[1]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nnue_network.accumulator_weights[u.b_deltas[0]][k] + nnue_network.accumulator_weights[u.b_deltas[1]][k];
			}
		} else if (u.deltas == 3) {
			// --+
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nnue_network.accumulator_weights[u.w_deltas[0]][k] - nnue_network.accumulator_weights[u.w_deltas[1]][k] + nnue_network.accumulator_weights[u.w_deltas[2]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nnue_network.accumulator_weights[u.b_deltas[0]][k] - nnue_network.accumulator_weights[u.b_deltas[1]][k] + nnue_network.accumulator_weights[u.b_deltas[2]][k];
			}
		} else if (u.deltas == 4) {
			// --++
			for (int k = 0; k < L1_SIZE; k++) {
				accs[i].w_acc.val[k] = accs[i-1].w_acc.val[k] - nnue_network.accumulator_weights[u.w_deltas[0]][k] - nnue_network.accumulator_weights[u.w_deltas[1]][k] + nnue_network.accumulator_weights[u.w_deltas[2]][k] + nnue_network.accumulator_weights[u.w_deltas[3]][k];
				accs[i].b_acc.val[k] = accs[i-1].b_acc.val[k] - nnue_network.accumulator_weights[u.b_deltas[0]][k] - nnue_network.accumulator_weights[u.b_deltas[1]][k] + nnue_network.accumulator_weights[u.b_deltas[2]][k] + nnue_network.accumulator_weights[u.b_deltas[3]][k];
			}
		}
		accs[i].correct = true;
	}
}

void AccumulatorManager::make_move(Position &pos, Move move, Position &pos_after) {
	idx++;
	AccumulatorPair &acc = accs[idx], &prev_acc = accs[idx - 1];
	acc.correct = false;

	if (move.type() == CASTLING || (pos.mailbox[move.src()] & 7) == KING) {
		// The king may have stepped into a new bucket, let's check to make sure
		int dest = move.dst();
		if (move.type() == CASTLING) {
			if (pos.side == WHITE) {
				dest = move.src() < move.dst() ? SQ_G1 : SQ_C1;
			} else {
				dest = move.src() < move.dst() ? SQ_G8 : SQ_C8;
			}
		}

		int prev_bucket = IBUCKET_LAYOUT[move.src() ^ (pos.side ? 56 : 0)];
		int new_bucket = IBUCKET_LAYOUT[dest ^ (pos.side ? 56 : 0)];
		if (prev_bucket != new_bucket) {
			refresh_finny(pos_after, idx);
			return;
		}
	}

	int winbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])];
	int binbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]) ^ 56];
	acc.winbucket = winbucket;
	acc.binbucket = binbucket;

	// 5 cases: quiet, promo, capture, en passant, castling
	bool promo = move.type() == PROMOTION;
	bool capture = pos.is_capture(move);
	bool ep = move.type() == EN_PASSANT;
	bool castle = move.type() == CASTLING;

	if (castle) {
		Square king_dest, rook_dest;
		if (pos.side == WHITE) {
			king_dest = move.src() < move.dst() ? SQ_G1 : SQ_C1;
			rook_dest = move.src() < move.dst() ? SQ_F1 : SQ_D1;
		} else {
			king_dest = move.src() < move.dst() ? SQ_G8 : SQ_C8;
			rook_dest = move.src() < move.dst() ? SQ_F8 : SQ_D8;
		}
		// 4 updates: rm king, rm rook, add king, add rook
		int windex1 = calculate_index(move.src(), KING, pos.side, 0, winbucket);
		int bindex1 = calculate_index(move.src(), KING, pos.side, 1, binbucket);
		int windex2 = calculate_index(move.dst(), ROOK, pos.side, 0, winbucket);
		int bindex2 = calculate_index(move.dst(), ROOK, pos.side, 1, binbucket);
		int windex3 = calculate_index(king_dest, KING, pos.side, 0, winbucket);
		int bindex3 = calculate_index(king_dest, KING, pos.side, 1, binbucket);
		int windex4 = calculate_index(rook_dest, ROOK, pos.side, 0, winbucket);
		int bindex4 = calculate_index(rook_dest, ROOK, pos.side, 1, binbucket);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3, windex4, bindex4};
		return;
	}

	if (ep) {
		// 3 updates: rm pawn, rm taken pawn, add pawn
		Square taken_pawn = Square((move.src() & 0b111000) | (move.dst() & 0b000111));
		int windex1 = calculate_index(move.src(), PAWN, pos.side, 0, winbucket);
		int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1, binbucket);
		int windex2 = calculate_index(taken_pawn, PAWN, !pos.side, 0, winbucket);
		int bindex2 = calculate_index(taken_pawn, PAWN, !pos.side, 1, binbucket);
		int windex3 = calculate_index(move.dst(), PAWN, pos.side, 0, winbucket);
		int bindex3 = calculate_index(move.dst(), PAWN, pos.side, 1, binbucket);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	if (promo) {
		if (!capture) {
			// 2 updates: rm pawn, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0, winbucket);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1, binbucket);
			int windex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0, winbucket);
			int bindex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1, binbucket);
			updates[idx] = {windex1, bindex1, windex2, bindex2};
		} else {
			// 3 updates: rm pawn, rm captured piece, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0, winbucket);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1, binbucket);
			PieceType captured_pt = PieceType(pos.mailbox[move.dst()] & 7);
			int windex2 = calculate_index(move.dst(), captured_pt, !pos.side, 0, winbucket);
			int bindex2 = calculate_index(move.dst(), captured_pt, !pos.side, 1, binbucket);
			int windex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0, winbucket);
			int bindex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1, binbucket);
			updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		}
		return;
	}

	if (capture) {
		// 3 updates: rm piece, rm captured, add piece
		int windex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
		int bindex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
		int windex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.dst()] & 7), !pos.side, 0, winbucket);
		int bindex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.dst()] & 7), !pos.side, 1, binbucket);
		int windex3 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
		int bindex3 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
		updates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	// 2 updates: rm piece, add piece
	int windex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
	int bindex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
	int windex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
	int bindex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
	updates[idx] = {windex1, bindex1, windex2, bindex2};
}
