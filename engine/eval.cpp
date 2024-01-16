#include "eval.hpp"

Value eval(const Board &board) {
	/*
	1. Si une côté manque un roi, return MATE
	2. Compte les matérials de chaque côté (pondéré à 4)
	3. Prendre une total des cases controllées par chaque côté (pondéré à 2)
	*/

	if (!(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])) {
		return VALUE_MATE;
	}
	if (!(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])) {
		return -VALUE_MATE;
	}

	Value material = 0;
	Value controlled = 0;

	material += PawnValue * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(WHITE)]);
	material += KnightValue * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(WHITE)]);
	material += BishopValue * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(WHITE)]);
	material += RookValue * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(WHITE)]);
	material += QueenValue * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(WHITE)]);
	material -= PawnValue * _mm_popcnt_u64(board.piece_boards[PAWN] & board.piece_boards[OCC(BLACK)]);
	material -= KnightValue * _mm_popcnt_u64(board.piece_boards[KNIGHT] & board.piece_boards[OCC(BLACK)]);
	material -= BishopValue * _mm_popcnt_u64(board.piece_boards[BISHOP] & board.piece_boards[OCC(BLACK)]);
	material -= RookValue * _mm_popcnt_u64(board.piece_boards[ROOK] & board.piece_boards[OCC(BLACK)]);
	material -= QueenValue * _mm_popcnt_u64(board.piece_boards[QUEEN] & board.piece_boards[OCC(BLACK)]);

	controlled += _mm_popcnt_u64(board.control[WHITE] & board.piece_boards[OCC(WHITE)]);
	controlled -= _mm_popcnt_u64(board.control[BLACK] & board.piece_boards[OCC(BLACK)]);

	return ((int)material * 4 + (int)controlled * 2) / 6;
}