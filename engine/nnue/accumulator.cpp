#include "accumulator.hpp"

void AccumulatorManager::AccumulatorPair::update_add(Square sq, PieceType pt, bool side, int wbucket, int bbucket) {
	uint16_t w_index = calculate_index(sq, pt, side, 0, wbucket);
	uint16_t b_index = calculate_index(sq, pt, side, 1, bbucket);
	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] += nnue_network.accumulator_weights[w_index][i];
		b_acc.val[i] += nnue_network.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::AccumulatorPair::update_sub(Square sq, PieceType pt, bool side, int wbucket, int bbucket) {
	uint16_t w_index = calculate_index(sq, pt, side, 0, wbucket);
	uint16_t b_index = calculate_index(sq, pt, side, 1, bbucket);
	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] -= nnue_network.accumulator_weights[w_index][i];
		b_acc.val[i] -= nnue_network.accumulator_weights[b_index][i];
	}
}

void AccumulatorManager::full_refresh(Position &pos, int index) {
	// Init the first accumulator so we have a basepoint
	for (int i = 0; i < HL_SIZE; i++) {
		accs[index].w_acc.val[i] = nnue_network.accumulator_biases[i];
		accs[index].b_acc.val[i] = nnue_network.accumulator_biases[i];
	}

	Square wkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

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

void AccumulatorManager::make_move(Position &pos, Move move, Position &pos_after) {
	AccumulatorPair &acc = accs[idx + 1], &prev_acc = accs[idx];
	acc.correct = true;
	idx++;

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
			full_refresh(pos_after, idx);
			return;
		}
	}

	// Copy previous accumulator as base for new one
	for (int i = 0; i < HL_SIZE; i++) {
		acc.w_acc.val[i] = prev_acc.w_acc.val[i];
		acc.b_acc.val[i] = prev_acc.b_acc.val[i];
	}

	int winbucket = IBUCKET_LAYOUT[_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])];
	int binbucket = IBUCKET_LAYOUT[_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]) ^ 56];

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
		acc.update_sub(move.src(), KING, pos.side, winbucket, binbucket);
		acc.update_sub(move.dst(), ROOK, pos.side, winbucket, binbucket);
		acc.update_add(king_dest, KING, pos.side, winbucket, binbucket);
		acc.update_add(rook_dest, ROOK, pos.side, winbucket, binbucket);
		return;
	}

	if (ep) {
		// 3 updates: rm pawn, rm taken pawn, add pawn
		Square taken_pawn = Square((move.src() & 0b111000) | (move.dst() & 0b000111));
		acc.update_sub(move.src(), PAWN, pos.side, winbucket, binbucket);
		acc.update_sub(taken_pawn, PAWN, !pos.side, winbucket, binbucket);
		acc.update_add(move.dst(), PAWN, pos.side, winbucket, binbucket);
		return;
	}

	if (capture) {
		// 1 update (so far): rm captured
		acc.update_sub(move.dst(), PieceType(pos.mailbox[move.dst()] & 7), !pos.side, winbucket, binbucket);
	}

	if (promo) {
		// 2 updates: rm pawn, add promo piece
		acc.update_sub(move.src(), PAWN, pos.side, winbucket, binbucket);
		acc.update_add(move.dst(), PieceType(move.promotion() + KNIGHT), pos.side, winbucket, binbucket);
		return;
	}

	// 2 updates: rm piece, add piece
	acc.update_sub(move.src(), PieceType(pos.mailbox[move.src()] & 7), pos.side, winbucket, binbucket);
	acc.update_add(move.dst(), PieceType(pos.mailbox[move.src()] & 7), pos.side, winbucket, binbucket);
}
