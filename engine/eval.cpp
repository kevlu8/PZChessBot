#include "eval.hpp"

Value eval(const Board &board) {
	/*
	1. Si une côté manque un roi, return MATE
	2. Compte les matérials de chaque côté (pondéré à 4)
	3. Prendre une total des cases controllées par chaque côté (pondéré à 2)
	*/

	if (!(board.pieces[KING] & board.pieces[OCC(BLACK)])) {
		return VALUE_MATE;
	}
	if (!(board.pieces[KING] & board.pieces[OCC(WHITE)])) {
		return -VALUE_MATE;
	}

	Value material = 0;
	Value controlled = 0;

	material += PawnValue * _mm_popcnt_u64(board.pieces[PAWN] & board.pieces[OCC(WHITE)]);
	material += KnightValue * _mm_popcnt_u64(board.pieces[KNIGHT] & board.pieces[OCC(WHITE)]);
	material += BishopValue * _mm_popcnt_u64(board.pieces[BISHOP] & board.pieces[OCC(WHITE)]);
	material += RookValue * _mm_popcnt_u64(board.pieces[ROOK] & board.pieces[OCC(WHITE)]);
	material += QueenValue * _mm_popcnt_u64(board.pieces[QUEEN] & board.pieces[OCC(WHITE)]);
	material -= PawnValue * _mm_popcnt_u64(board.pieces[PAWN] & board.pieces[OCC(BLACK)]);
	material -= KnightValue * _mm_popcnt_u64(board.pieces[KNIGHT] & board.pieces[OCC(BLACK)]);
	material -= BishopValue * _mm_popcnt_u64(board.pieces[BISHOP] & board.pieces[OCC(BLACK)]);
	material -= RookValue * _mm_popcnt_u64(board.pieces[ROOK] & board.pieces[OCC(BLACK)]);
	material -= QueenValue * _mm_popcnt_u64(board.pieces[QUEEN] & board.pieces[OCC(BLACK)]);

	controlled += _mm_popcnt_u64(board.control[WHITE] & board.pieces[OCC(WHITE)]);
	controlled -= _mm_popcnt_u64(board.control[BLACK] & board.pieces[OCC(BLACK)]);

	return ((int)material * 4 + (int)controlled * 2) / 6;
}