#include "eval.hpp"

Network *nnue_networks[8];

__attribute__((constructor)) void init_network() {
	// nnue_networks[0] = (Network *)numa_alloc_onnode(sizeof(Network), 0);
	// if (nnue_networks[0] == nullptr) {
	// 	std::cerr << "Mais qu'est-ce qui se passe?" << std::endl;
	// 	exit(1);
	// }
	// nnue_networks[0]->load();
	// if (numa_available() < 0) {
	// 	std::cout << "No NUMA detected - this is fine if you have one CPU." << std::endl;
	// 	return;
	// } else {
	// 	// If we have multiple NUMA nodes, load the second network on the second NUMA node
	// 	int tot_nodes = numa_max_node() + 1;
	// 	if (tot_nodes > 8) {
	// 		std::cerr << "More than 8 NUMA nodes detected. Performance may be impacted." << std::endl;
	// 		tot_nodes = 8;
	// 	}
	// 	for (int i = 1; i < tot_nodes; i++) {
	// 		nnue_networks[i] = (Network *)numa_alloc_onnode(sizeof(Network), i);
	// 		if (nnue_networks[i] == nullptr) {
	// 			std::cerr << "Pourquoi ton ordinateur veut pas fonctionner?" << std::endl;
	// 			nnue_networks[i] = nnue_networks[0];
	// 		} else {
	// 			nnue_networks[i]->load();
	// 		}
	// 	}
	// }
	nnue_networks[0] = new Network();
	nnue_networks[0]->load();
}

Value simple_eval(Position &pos) {
	Value score = 0;
	for (int i = 0; i < 6; i++) {
		score += PieceValue[i] * _mm_popcnt_u64(pos.piece_boards[i] & pos.piece_boards[OCC(WHITE)]);
		score -= PieceValue[i] * _mm_popcnt_u64(pos.piece_boards[i] & pos.piece_boards[OCC(BLACK)]);
	}
	return score;
}

Value eval(Position &pos, AccumulatorManager &am) {
	int npieces = _mm_popcnt_u64(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);
	int32_t score = 0;

	am.apply_lazy(pos);

	int nbucket = (npieces - 2) / 4;

	if (pos.side == WHITE) {
		score = nnue_eval(am.current().w_acc, am.current().b_acc, nbucket);
	} else {
		score = -nnue_eval(am.current().b_acc, am.current().w_acc, nbucket);
	}
	
	const int mat_phase = PawnValue * _mm_popcnt_u64(pos.piece_boards[PAWN])
						+ KnightValue * _mm_popcnt_u64(pos.piece_boards[KNIGHT])
						+ BishopValue * _mm_popcnt_u64(pos.piece_boards[BISHOP])
						+ RookValue * _mm_popcnt_u64(pos.piece_boards[ROOK])
						+ QueenValue * _mm_popcnt_u64(pos.piece_boards[QUEEN]);
	
	return score * (26500 + mat_phase) / 32768;
}

std::array<Value, 8> debug_eval(Position &pos) {
	if (!(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)])) {
		// If black has no king, this is mate for white
		return {VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (!(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)])) {
		// Likewise, if white has no king, this is mate for black
		return {-VALUE_MATE, 0, 0, 0, 0, 0, 0, 0};
	}
	if (pos.halfmove >= 100) {
		return {0, 0, 0, 0, 0, 0, 0, 0}; // Draw by 50 moves
	}

	Square wkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(WHITE)]);
	Square bkingsq = (Square)_tzcnt_u64(pos.piece_boards[KING] & pos.piece_boards[OCC(BLACK)]);
	int winbucket = IBUCKET_LAYOUT[wkingsq];
	int binbucket = IBUCKET_LAYOUT[bkingsq ^ 56];

	AccumulatorManager am(pos);

	int npieces = _mm_popcnt_u64(pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)]);

	std::array<Value, 8> score = {};
	if (pos.side == WHITE) {
		for (int i = 0; i < 8; i++) {
			score[i] = nnue_eval(am.current().w_acc, am.current().b_acc, i);
		}
	} else {
		for (int i = 0; i < 8; i++) {
			score[i] = -nnue_eval(am.current().b_acc, am.current().w_acc, i);
		}
	}

	return score;
}
