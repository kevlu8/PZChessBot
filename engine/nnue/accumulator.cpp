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

	refresh_threats(pos, index, WHITE);
	refresh_threats(pos, index, BLACK);
	accs[index].correct[WHITE] = accs[index].correct[BLACK] = true;
}

void AccumulatorManager::refresh_threats(Position &pos, int index, bool perspective) {
	Accumulator &acc = perspective == WHITE ? accs[index].w_threats : accs[index].b_threats;
	std::fill(acc.val, acc.val + L1_SIZE, 0);

	Square kingsq = (Square)arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(perspective)]);
	Bitboard occ = pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)];
	for (int i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		if (piece == NO_PIECE) continue;

		Bitboard attacks = calc_attacks(piece, (Square)i, occ) & occ;
		while (attacks) {
			Square dst = (Square)arch::tzcnt(attacks);
			int t_index = threat_index(perspective, kingsq, piece, pos.mailbox[dst], (Square)i, dst);
			if (t_index >= 0) {
				for (int k = 0; k < L1_SIZE; k++) {
					acc.val[k] += nnue_network.threat_weights[t_index][k];
				}
			}
			attacks = arch::blsr(attacks);
		}
	}
	accs[index].threats_correct[perspective] = true;
}

void AccumulatorManager::refresh_finny(Position &pos, int index, bool perspective) {
	int bucket = perspective == WHITE ? accs[index].winbucket : accs[index].binbucket;
	Accumulator &cached = perspective == WHITE ? finny.accs[bucket].w_acc : finny.accs[bucket].b_acc;
	Accumulator &acc = perspective == WHITE ? accs[index].w_acc : accs[index].b_acc;

	for (int i = 0; i < 64; i++) {
		Piece piece = pos.mailbox[i];
		Piece prev_piece = finny.mailboxes[bucket][perspective][i];
		if (piece == prev_piece) continue;

		if (piece != NO_PIECE) {
			int p_index = calculate_index((Square)i, PieceType(piece & 7), piece >> 3, perspective, bucket);
			for (int k = 0; k < L1_SIZE; k++) {
				cached.val[k] += nnue_network.accumulator_weights[p_index][k];
			}
		}
		if (prev_piece != NO_PIECE) {
			int p_index = calculate_index((Square)i, PieceType(prev_piece & 7), prev_piece >> 3, perspective, bucket);
			for (int k = 0; k < L1_SIZE; k++) {
				cached.val[k] -= nnue_network.accumulator_weights[p_index][k];
			}
		}
		finny.mailboxes[bucket][perspective][i] = piece;
	}
	acc = cached;
	accs[index].correct[perspective] = true;
}

void AccumulatorManager::apply_lazy(Position &pos) {
	for (int perspective = WHITE; perspective <= BLACK; perspective++) {
		if (!current().correct[perspective]) {
			int index = idx;
			while (index > 0 && !accs[index].correct[perspective]) {
				int bucket = perspective == WHITE ? accs[index].winbucket : accs[index].binbucket;
				int prev_bucket = perspective == WHITE ? accs[index-1].winbucket : accs[index-1].binbucket;
				if (bucket != prev_bucket) break;
				index--;
			}

			if (!accs[index].correct[perspective]) {
				refresh_finny(pos, idx, perspective);
			} else {
				for (int i = index + 1; i <= idx; i++) {
					auto &u = psqtupdates[i];
					int *deltas = perspective == WHITE ? u.w_deltas : u.b_deltas;
					Accumulator &acc = perspective == WHITE ? accs[i].w_acc : accs[i].b_acc;
					Accumulator &prev = perspective == WHITE ? accs[i-1].w_acc : accs[i-1].b_acc;
					if (u.deltas == 2) {
						// -+
						for (int k = 0; k < L1_SIZE; k++) {
							acc.val[k] = prev.val[k] - nnue_network.accumulator_weights[deltas[0]][k] + nnue_network.accumulator_weights[deltas[1]][k];
						}
					} else if (u.deltas == 3) {
						// --+
						for (int k = 0; k < L1_SIZE; k++) {
							acc.val[k] = prev.val[k] - nnue_network.accumulator_weights[deltas[0]][k] - nnue_network.accumulator_weights[deltas[1]][k] + nnue_network.accumulator_weights[deltas[2]][k];
						}
					} else if (u.deltas == 4) {
						// --++
						for (int k = 0; k < L1_SIZE; k++) {
							acc.val[k] = prev.val[k] - nnue_network.accumulator_weights[deltas[0]][k] - nnue_network.accumulator_weights[deltas[1]][k] + nnue_network.accumulator_weights[deltas[2]][k] + nnue_network.accumulator_weights[deltas[3]][k];
						}
					}
					accs[i].correct[perspective] = true;
				}
			}
		}

		if (!current().threats_correct[perspective]) {
			int index = idx;
			while (index > 0 && !accs[index].threats_correct[perspective]) {
				int bucket = perspective == WHITE ? accs[index].winbucket : accs[index].binbucket;
				int prev_bucket = perspective == WHITE ? accs[index-1].winbucket : accs[index-1].binbucket;
				// Threat indices only change when the king crosses the horizontal mirror
				if ((bucket ^ prev_bucket) & 1) break;
				index--;
			}

			if (!accs[index].threats_correct[perspective]) {
				refresh_threats(pos, idx, perspective);
			} else {
				for (int i = index + 1; i <= idx; i++) {
					auto &tu = threatupdates[i];
					Accumulator &acc = perspective == WHITE ? accs[i].w_threats : accs[i].b_threats;
					acc = perspective == WHITE ? accs[i-1].w_threats : accs[i-1].b_threats;
					int *adds = perspective == WHITE ? tu.w_adds : tu.b_adds;
					int *removes = perspective == WHITE ? tu.w_removes : tu.b_removes;
					int nadds = perspective == WHITE ? tu.widxa : tu.bidxa;
					int nremoves = perspective == WHITE ? tu.widxr : tu.bidxr;
					for (int j = 0; j < nadds; j++) {
						for (int k = 0; k < L1_SIZE; k++) {
							acc.val[k] += nnue_network.threat_weights[adds[j]][k];
						}
					}
					for (int j = 0; j < nremoves; j++) {
						for (int k = 0; k < L1_SIZE; k++) {
							acc.val[k] -= nnue_network.threat_weights[removes[j]][k];
						}
					}
					accs[i].threats_correct[perspective] = true;
				}
			}
		}
	}
}

void AccumulatorManager::make_move(Position &pos, Move move, Position &pos_after) {
	idx++;
	AccumulatorPair &acc = accs[idx];
	acc.correct[WHITE] = acc.correct[BLACK] = false;
	acc.threats_correct[WHITE] = acc.threats_correct[BLACK] = false;
	auto &tu = threatupdates[idx];
	tu.clear();

	int winbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])];
	int binbucket = IBUCKET_LAYOUT[arch::tzcnt(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]) ^ 56];
	acc.winbucket = IBUCKET_LAYOUT[arch::tzcnt(pos_after.piece_boards[KING] & pos_after.piece_boards[OCC(WHITE)])];
	acc.binbucket = IBUCKET_LAYOUT[arch::tzcnt(pos_after.piece_boards[KING] & pos_after.piece_boards[OCC(BLACK)]) ^ 56];

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
