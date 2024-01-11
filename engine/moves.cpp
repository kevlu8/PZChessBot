#include "moves.hpp"

uint64_t pawn_moves_table[128];

void pawn_moves(const Board &board, std::vector<Move> &moves, bool side) {
	uint64_t pieces = board.pieces[PAWN] & board.pieces[OCC(side)];

	while (pieces) {
		// Find the lowest bit and put it in pawn_bit
		uint64_t pawn_bit = _blsi_u64(pieces);
		// Get the square value of that bit
		Square sq = Square(_tzcnt_u64(pawn_bit));
		// Clear the bit in the pieces bitboard
		pieces ^= pawn_bit;

		// Use a table
		uint64_t dests = pawn_moves_table[sq | (side << 6)];

		while (dests) {
			uint64_t dest_bit = _blsi_u64(dests);
			MoveType t = NORMAL;

			if (dest_bit & RANK_1 || dest_bit & RANK_8)
				t = PROMOTION;

			if (dest_bit == square_bits(board.ep_square))
				t = EN_PASSANT;
		}
	}

	// OK il y a une problème ici
	// si on utilise une table pour mapper les src aux dst on n'a aucune façon de savoir si c'est une mange ou non
	// et si c'est une mange on doit vérifier qu'il y a vraiment un pièce dans le dst
	// mais si c'est pas une mange on doit vérifier qu'il n'y a aucun pièce
	// alors on a une problème
}
