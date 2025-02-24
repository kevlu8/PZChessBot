#include "search.hpp"

uint64_t nodes = 0;

void _perft(Board &board, int depth) {
	if (depth == 0) {
		nodes++;
		return;
	}
	std::vector<Move> moves;
	board.legal_moves(moves);
	for (Move &move : moves) {
		if (_mm_popcnt_u64(board.piece_boards[KING]) == 2)
			_perft(board, depth - 1);
		board.unmake_move();
	}
}

uint64_t perft(Board &board, int depth) {
	uint64_t total = 0;
	std::vector<Move> moves;
	board.legal_moves(moves);
	for (Move &move : moves) {
		nodes = 0;
		board.make_move(move);
		_perft(board, depth - 1);
		board.unmake_move();
		total += nodes;
		std::cout << move.to_string() << ": " << nodes << std::endl;
	}
	return total;
}

bool thing[10];

Value __recurse(Board &board, int depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE, int side = 1) {
	if (depth <= 0) {
		nodes++;
#ifndef PERFT // We don't need to compile eval.cpp if we are only doing perfts
		return eval(board) * side;
#endif
	}

	std::vector<Move> moves;
	board.legal_moves(moves);

	Value best = -VALUE_INFINITE;

	// Move ordering (experimental)
	if (depth > 2) {
		std::vector<std::pair<Move, Value>> scores;
		for (int i = 0; i < moves.size(); i++) {
			Move &move = moves[i];
			board.make_move(move);
			Value score = eval(board) * side;
			board.unmake_move();
			scores.push_back({move, score});
		}
		std::sort(scores.begin(), scores.end(), [](const std::pair<Move, Value> &a, const std::pair<Move, Value> &b) { return a.second > b.second; });
		moves.clear();
		for (int i = 0; i < scores.size(); i++) {
			moves.push_back(scores[i].first);
		}
	}

	for (int i = 0; i < moves.size(); i++) {
		Move &move = moves[i];
		board.make_move(move);

		// if (thing[0] && depth == 5)
		// 	thing[1] = true;
		// if (thing[0] && thing[1] && depth == 4)
		// 	thing[2] = move.data == 0b101111111101;

		// Value score = -__recurse(board, (i > 15 && depth > 2) ? depth - 2 : depth - 1, -beta, -alpha, -side);
		Value score = -__recurse(board, depth - 1, -beta, -alpha, -side);

		// if (thing[0] && depth == 5)
		// 	std::cout << "\x1b[33m" << move.to_string() << ' ' << score << ' ' << alpha << ' ' << beta << "\x1b[0m\n";
		// if (thing[0] && thing[1] && depth == 4)
		// 	std::cout << "\x1b[31m" << move.to_string() << ' ' << score << ' ' << alpha << ' ' << beta << "\x1b[0m\n";
		// if (thing[0] && thing[1] && thing[2] && depth == 3)
		// 	std::cout << "\x1b[32m" << move.to_string() << ' ' << score << ' ' << alpha << ' ' << beta << "\x1b[0m\n";

		score = score - ((score >> 15) << 1) - 1;
		board.unmake_move();

		if (score > best) {
			if (score > alpha) {
				alpha = score;
			}
			best = score;
		}
		if (score >= beta) {
			return best;
		}
	}

	// return alpha;
	return best;
}

std::pair<Move, Value> search(Board &board, int depth) {
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
		// thing[0] = move.data == 0b011111101111;
		thing[0] = move.data == 0b010000011001;
		// thing[1] = move.data == 0b000110000111;
		Value score = -__recurse(board, depth - 1, -beta, -alpha, board.side ? -1 : 1);
		board.unmake_move();

		// std::cout << move.to_string() << " " << score << '\n';

		if (score + (rand() & 1) > best_score) {
			if (score > alpha) {
				alpha = score;
			}
			best_score = score;
			best_move = move;
		}

		if (score >= beta) {
			std::cout << "broke\n";
			break;
		}
	}

	std::cout << "nodes " << nodes << " nps " << (nodes / ((double)(clock() - start) / CLOCKS_PER_SEC)) << std::endl;

	return {best_move, best_score};
}
