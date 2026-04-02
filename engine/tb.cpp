#include "tb.hpp"

std::optional<int> TBManager::probe_pos(Position &pos) {
	if (!initialized || pos.halfmove != 0 || pos.castling) {
		return std::nullopt;
	}

	int npieces = _mm_popcnt_u64(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	if (npieces > TB_LARGEST || npieces > max_pieces) {
		return std::nullopt;
	}

	auto ep_sq = pos.ep_square != SQ_NONE ? pos.ep_square : 0;
	auto turn = pos.side == WHITE ? PYRRHIC_WHITE : PYRRHIC_BLACK;

	unsigned res = tb_probe_wdl(pos.piece_boards[OCC(WHITE)], pos.piece_boards[OCC(BLACK)],
		pos.piece_boards[KING], pos.piece_boards[QUEEN], pos.piece_boards[ROOK], pos.piece_boards[BISHOP],
		pos.piece_boards[KNIGHT], pos.piece_boards[PAWN], ep_sq, turn);

	if (res == TB_RESULT_FAILED) {
		std::cerr << "Je sais pas ce qui s'est passé mais je sais que t'es un fils de pute" << std::endl;
		return std::nullopt;
	}

	if (res == TB_WIN) return 1;
	if (res == TB_LOSS) return -1;
	return 0; // draws, cursed wins, and blessed losses are all treated as draws
}

TBManager tbman;
