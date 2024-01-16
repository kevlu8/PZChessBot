#include "moves.hpp"

void white_pawn_moves(const Board &board, std::vector<Move> &moves, bool side) {
	uint64_t pieces = board.piece_boards[PAWN] & board.piece_boards[OCC(side)];
	// Normal single pushes (no promotion)
	uint64_t dsts = ((pieces & ~Rank7Bits) << 8) & ~(board.piece_boards[OCC(side)] | board.piece_boards[OPPOCC(side)]);
	uint64_t tmp = dsts;
	while (tmp) {
		int sq = _tzcnt_u64(tmp);
		tmp ^= 1ULL << sq;
		moves.push_back(Move(sq - 8, sq));
	}
	// Double pushes
	dsts = ((dsts & Rank3Bits) << 8) & ~(board.piece_boards[OCC(side)] | board.piece_boards[OPPOCC(side)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move(sq - 16, sq));
	}
	// Promotion
	dsts = ((pieces & Rank7Bits) << 8) & ~(board.piece_boards[OCC(side)] | board.piece_boards[OPPOCC(side)]);
	while (dsts) {
		int sq = _tzcnt_u64(dsts);
		dsts ^= 1ULL << sq;
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, QUEEN));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, ROOK));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, KNIGHT));
		moves.push_back(Move::make<PROMOTION>(sq - 8, sq, BISHOP));
	}
	/// TODO: Add captures and EP
}
