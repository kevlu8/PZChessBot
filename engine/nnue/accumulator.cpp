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

void compute_threat_updates(Position &before, Position &after, Bitboard changed, AccumulatorManager::ThreatUpdate &tu) {
	Square wkingsq = (Square)arch::tzcnt(before.piece_boards[KING] & before.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)arch::tzcnt(before.piece_boards[KING] & before.piece_boards[OCC(BLACK)]);

	Bitboard before_occ = before.piece_boards[OCC(WHITE)] | before.piece_boards[OCC(BLACK)];
	Bitboard after_occ = after.piece_boards[OCC(WHITE)] | after.piece_boards[OCC(BLACK)];

	auto attackers_to = [](Position &pos, Square sq, Bitboard occ) {
		return (rook_attacks(sq, occ) & (pos.piece_boards[ROOK] | pos.piece_boards[QUEEN]))
		     | (bishop_attacks(sq, occ) & (pos.piece_boards[BISHOP] | pos.piece_boards[QUEEN]))
		     | (knight_attacks(sq) & pos.piece_boards[KNIGHT])
		     | (pawn_attacks(sq, BLACK) & pos.piece_boards[PAWN] & pos.piece_boards[OCC(WHITE)])
		     | (pawn_attacks(sq, WHITE) & pos.piece_boards[PAWN] & pos.piece_boards[OCC(BLACK)]);
	};

	// Handle squares that changed occ by the move
	Bitboard affected = changed, squares = changed;
	while (squares) {
		Square sq = (Square)arch::tzcnt(squares);
		affected |= attackers_to(before, sq, before_occ) | attackers_to(after, sq, after_occ);

		squares = arch::blsr(squares);
	}
	affected &= before_occ | after_occ; // only care about squares that have pieces

	while (affected) {
		Square src = (Square)arch::tzcnt(affected);

		Piece old_piece = before.mailbox[src];
		Piece new_piece = after.mailbox[src];

		Bitboard old_attacks = calc_attacks(old_piece, src, before_occ) & before_occ;
		Bitboard new_attacks = calc_attacks(new_piece, src, after_occ) & after_occ;

		// if the piece didn't move then we only care about the attacks that changed, otherwise recompute all the attacks
		Bitboard removed = old_piece != new_piece ? old_attacks : old_attacks & (~new_attacks | changed);
		Bitboard added = old_piece != new_piece ? new_attacks : new_attacks & (~old_attacks | changed);
		while (removed) {
			Square dst = (Square)arch::tzcnt(removed);
			removed = arch::blsr(removed);
			int w_t_index = threat_index(WHITE, wkingsq, old_piece, before.mailbox[dst], src, dst);
			int b_t_index = threat_index(BLACK, bkingsq, old_piece, before.mailbox[dst], src, dst);
			if (w_t_index >= 0) tu.remove_white(w_t_index);
			if (b_t_index >= 0) tu.remove_black(b_t_index);
		}
		while (added) {
			Square dst = (Square)arch::tzcnt(added);
			added = arch::blsr(added);
			int w_t_index = threat_index(WHITE, wkingsq, new_piece, after.mailbox[dst], src, dst);
			int b_t_index = threat_index(BLACK, bkingsq, new_piece, after.mailbox[dst], src, dst);
			if (w_t_index >= 0) tu.add_white(w_t_index);
			if (b_t_index >= 0) tu.add_black(b_t_index);
		}

		affected = arch::blsr(affected);
	}
}

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

void AccumulatorManager::AccumulatorPair::update_white_threat_add(int index) {
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] += nnue_network.threat_weights[index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_black_threat_add(int index) {
	for (int i = 0; i < L1_SIZE; i++) {
		b_acc.val[i] += nnue_network.threat_weights[index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_white_threat_sub(int index) {
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] -= nnue_network.threat_weights[index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_black_threat_sub(int index) {
	for (int i = 0; i < L1_SIZE; i++) {
		b_acc.val[i] -= nnue_network.threat_weights[index][i];
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

	// Update threats
	for (uint16_t i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		bool side = piece >> 3; // 1 = black, 0 = white
		PieceType pt = PieceType(piece & 7);

		if (piece != NO_PIECE) {
			// Find the pieces that this piece threatens and loop through them
			Bitboard attacks = calc_attacks(piece, (Square)i, pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
			attacks &= (pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]); // Only consider squares that have pieces on them
			while (attacks) {
				Square target_sq = (Square)arch::tzcnt(attacks);
				Piece target_piece = pos.mailbox[target_sq];
				bool target_side = target_piece >> 3;
				PieceType target_pt = PieceType(target_piece & 7);

				int w_t_index = threat_index(0, wkingsq, piece, target_piece, (Square)i, target_sq);
				int b_t_index = threat_index(1, bkingsq, piece, target_piece, (Square)i, target_sq);
				// std::cout << "White: " << piece_letter[piece] << " at " << (int)i << " threatens " << piece_letter[target_piece] << " at " << (int)target_sq << " with index " << w_t_index << std::endl;
				// std::cout << "Black: " << piece_letter[piece] << " at " << (int)i << " threatens " << piece_letter[target_piece] << " at " << (int)target_sq << " with index " << b_t_index << std::endl;

				if (w_t_index >= 0)
					accs[index].update_white_threat_add(w_t_index);
				if (b_t_index >= 0)
					accs[index].update_black_threat_add(b_t_index);

				attacks = arch::blsr(attacks);
			}
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
					f_w_acc.val[k] += nnue_network.accumulator_weights[index][k];
				}
			}

			if (prev_w_piece != NO_PIECE) {
				// Remove from accumulator
				int index = calculate_index((Square)i, prev_w_pt, prev_w_side, 0, winbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					f_w_acc.val[k] -= nnue_network.accumulator_weights[index][k];
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
					f_b_acc.val[k] += nnue_network.accumulator_weights[index][k];
				}
			}

			if (prev_b_piece != NO_PIECE) {
				// Remove from accumulator
				int index = calculate_index((Square)i, prev_b_pt, prev_b_side, 1, binbucket);
				for (int k = 0; k < L1_SIZE; k++) {
					f_b_acc.val[k] -= nnue_network.accumulator_weights[index][k];
				}
			}
		}
	}

	// Update accumulators
	for (int i = 0; i < L1_SIZE; i++) {
		w_acc.val[i] = f_w_acc.val[i];
		b_acc.val[i] = f_b_acc.val[i];
	}

	// such a small loop that it's not worth incrementally doing
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
		full_refresh(pos, idx); // was refresh_finny() for non-threat inputs but idk how to do finny + threats
		return;
	}

	for (int i = index + 1; i <= idx; i++) {
		auto &u = psqtupdates[i];
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

		auto &tu = threatupdates[i];
		for (int j = 0; j < tu.widxa; j++) {
			accs[i].update_white_threat_add(tu.w_adds[j]);
		}
		for (int j = 0; j < tu.widxr; j++) {
			accs[i].update_white_threat_sub(tu.w_removes[j]);
		}
		for (int j = 0; j < tu.bidxa; j++) {
			accs[i].update_black_threat_add(tu.b_adds[j]);
		}
		for (int j = 0; j < tu.bidxr; j++) {
			accs[i].update_black_threat_sub(tu.b_removes[j]);
		}

		accs[i].correct = true;
	}
}

void AccumulatorManager::make_move(Position &pos, Move move, Position &pos_after) {
	idx++;
	AccumulatorPair &acc = accs[idx];
	acc.correct = false;
	auto &tu = threatupdates[idx];
	tu.clear();

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
			full_refresh(pos_after, idx); // same thing here, was refresh_finny
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

	Bitboard changed = square_bits(move.src()) | square_bits(move.dst());
	if (ep) {
		changed |= square_bits(Square((move.src() & 0b111000) | (move.dst() & 0b000111)));
	} else if (castle) {
		int rank = pos.side == WHITE ? 0 : 56;
		changed |= square_bits(Square(rank + (move.src() < move.dst() ? SQ_G1 : SQ_C1)));
		changed |= square_bits(Square(rank + (move.src() < move.dst() ? SQ_F1 : SQ_D1)));
	}
	compute_threat_updates(pos, pos_after, changed, tu);

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
		psqtupdates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3, windex4, bindex4};
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
		psqtupdates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	if (promo) {
		if (!capture) {
			// 2 updates: rm pawn, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0, winbucket);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1, binbucket);
			int windex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0, winbucket);
			int bindex2 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1, binbucket);
			psqtupdates[idx] = {windex1, bindex1, windex2, bindex2};
		} else {
			// 3 updates: rm pawn, rm captured piece, add promo piece
			int windex1 = calculate_index(move.src(), PAWN, pos.side, 0, winbucket);
			int bindex1 = calculate_index(move.src(), PAWN, pos.side, 1, binbucket);
			PieceType captured_pt = PieceType(pos.mailbox[move.dst()] & 7);
			int windex2 = calculate_index(move.dst(), captured_pt, !pos.side, 0, winbucket);
			int bindex2 = calculate_index(move.dst(), captured_pt, !pos.side, 1, binbucket);
			int windex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 0, winbucket);
			int bindex3 = calculate_index(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, 1, binbucket);
			psqtupdates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
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
		psqtupdates[idx] = {windex1, bindex1, windex2, bindex2, windex3, bindex3};
		return;
	}

	// 2 updates: rm piece, add piece
	int windex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
	int bindex1 = calculate_index(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
	int windex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 0, winbucket);
	int bindex2 = calculate_index(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, 1, binbucket);
	psqtupdates[idx] = {windex1, bindex1, windex2, bindex2};
}
