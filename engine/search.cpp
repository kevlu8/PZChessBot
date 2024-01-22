#include "search.hpp"

Bitboard nodes = 0;

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	if (depth <= 0) {
		nodes++;
		return eval(board) * side;
	}

	std::vector<Move> moves;
	board.legal_moves(moves);

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);
		Value score = -__recurse(board, (i > 15 ? depth - 2 : depth - 1), -beta, -alpha, -side);
		board.unmake_move();

		if (score >= beta) {
			return beta;
		}

		if (score > alpha) {
			alpha = score;
		}
	}

	return alpha;
}

Move search(Board &board, int depth) {
	nodes = 0;

	clock_t start = clock();

	std::vector<Move> moves;
	board.legal_moves(moves);

	Move best_move = moves[0];
	Value best_score = -VALUE_INFINITE;

	Value alpha = -VALUE_INFINITE;
	Value beta = VALUE_INFINITE;

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);
		Value score = -__recurse(board, depth - 1, alpha, beta);
		board.unmake_move();

		if (score > best_score) {
			best_score = score;
			best_move = move;
		}

		if (score >= beta) {
			return best_move;
		}

		if (score > alpha) {
			alpha = score;
		}
	}

	std::cout << "nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << std::endl;

	return best_move;
}
