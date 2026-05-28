#include "tb.hpp"

std::optional<int> TBManager::probe_pos(Position &pos) {
	if (!initialized || pos.halfmove != 0 || pos.castling) {
		return std::nullopt;
	}

	int npieces = __builtin_popcountll(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	if (npieces > TB_LARGEST || npieces > max_pieces) {
		return std::nullopt;
	}

	auto ep_sq = pos.ep_square != SQ_NONE ? pos.ep_square : 0;
	auto turn = pos.side == WHITE ? PYRRHIC_WHITE : PYRRHIC_BLACK;

	unsigned res = tb_probe_wdl(pos.piece_boards[OCC(WHITE)], pos.piece_boards[OCC(BLACK)],
		pos.piece_boards[KING], pos.piece_boards[QUEEN], pos.piece_boards[ROOK], pos.piece_boards[BISHOP],
		pos.piece_boards[KNIGHT], pos.piece_boards[PAWN], ep_sq, turn);

	if (res == TB_RESULT_FAILED) {
		// Probably due to an incomplete set of Syzygy bases
		return std::nullopt;
	}

	if (res == TB_WIN) return 1;
	if (res == TB_LOSS) return -1;
	return 0; // draws, cursed wins, and blessed losses are all treated as draws
}

std::unordered_set<uint16_t> TBManager::probe_moves(Position &pos, bool rep) {
	std::unordered_set<uint16_t> viable_moves;
	if (!initialized || pos.castling) {
		return viable_moves;
	}

	int npieces = __builtin_popcountll(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	if (npieces > TB_LARGEST || npieces > max_pieces) {
		return viable_moves;
	}

	auto ep_sq = pos.ep_square != SQ_NONE ? pos.ep_square : 0;
	auto turn = pos.side == WHITE ? PYRRHIC_WHITE : PYRRHIC_BLACK;

	TbRootMoves moves = {};
	int res = tb_probe_root_dtz(pos.piece_boards[OCC(WHITE)], pos.piece_boards[OCC(BLACK)],
		pos.piece_boards[KING], pos.piece_boards[QUEEN], pos.piece_boards[ROOK], pos.piece_boards[BISHOP],
		pos.piece_boards[KNIGHT], pos.piece_boards[PAWN], pos.halfmove, ep_sq, turn, rep, &moves);
	
	if (res == 0) {
		return viable_moves;
	}

	int optimal_score = -2147483647;
	for (int i = 0; i < moves.size; i++) {
		int score = moves.moves[i].tbRank;
		optimal_score = std::max(optimal_score, score);
	}

	for (int i = 0; i < moves.size; i++) {
		int score = moves.moves[i].tbRank;
		if (score == optimal_score) {
			auto move = moves.moves[i].move;
			
			Move pz_move = Move(PYRRHIC_MOVE_FROM(move), PYRRHIC_MOVE_TO(move));
			if (PYRRHIC_MOVE_IS_ENPASS(move)) pz_move.data |= (2 << 14);
			else if (PYRRHIC_MOVE_IS_QPROMO(move)) pz_move.data |= (1 << 14) | ((QUEEN - KNIGHT) << 12);
			else if (PYRRHIC_MOVE_IS_RPROMO(move)) pz_move.data |= (1 << 14) | ((ROOK - KNIGHT) << 12);
			else if (PYRRHIC_MOVE_IS_BPROMO(move)) pz_move.data |= (1 << 14) | ((BISHOP - KNIGHT) << 12);
			else if (PYRRHIC_MOVE_IS_NPROMO(move)) pz_move.data |= (1 << 14) | ((KNIGHT - KNIGHT) << 12);

			viable_moves.insert(pz_move.data);
		}
	}

	return viable_moves;
}

TBManager tbman;
