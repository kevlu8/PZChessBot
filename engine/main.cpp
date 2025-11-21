#include "includes.hpp"

#include <random>
#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"
#include "ttable.hpp"

BoardState bs[MAX_THREADS][NINPUTS * 2][NINPUTS * 2];
ThreadInfo ti[MAX_THREADS];

void datagen(ThreadInfo &ti) {
	std::fstream outfile(std::to_string(ti.id) + "_datagen.pgn", std::ios::out | std::ios::app);
	int games = 0;
	std::mt19937_64 rng(ti.id + std::chrono::system_clock::now().time_since_epoch().count());
	while (true) {
		clear_search_vars(ti);
		Board &board = ti.board;

		for (int i = 0; i < DATAGEN_NUM_RAND; i++) {
			pzstd::vector<Move> moves;
			board.legal_moves(moves);
			if (moves.size() == 0) break;
			board.make_move(moves[rng() % moves.size()]); // technically bad distribution but who cares
		}

		bool in_check = false, checking_opponent = false;
		if (board.side == WHITE) {
			in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK);
			checking_opponent = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE);
		} else {
			in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE);
			checking_opponent = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK);
		}
		if (in_check || checking_opponent) continue;
		if (_mm_popcnt_u64(board.piece_boards[KING]) != 2) continue;
		auto s_eval = eval(board, ti.bs);
		if (abs(s_eval) >= 600) continue; // ridiculous position
		auto res = search(ti).second;
		if (abs(res) >= 400) continue; // unbalanced position

		std::string startfen = board.get_fen();

		std::vector<std::string> movelist;

		int consec_draw = 0, consec_win = 0;
		int plies = DATAGEN_NUM_RAND;
		while (true) {
			auto result = search(ti);
			Move bestmove = result.first;
			Value score = result.second;

			if (score < 20 && plies >= 80) consec_draw++;
			else consec_draw = 0;
			
		}
	}
}

int main(int argc, char *argv[]) {
	// ./pzchessbot [num_threads]
	int n_threads = 1;
	if (argc >= 2) {
		n_threads = std::stoi(argv[1]);
		if (n_threads < 1 || n_threads > MAX_THREADS) {
			std::cerr << "Invalid number of threads" << std::endl;
			return 1;
		}
	}

	for (int i = 0; i < n_threads; i++) {
		ti[i].id = i;
		ti[i].bs = (void*)&bs[i];
		ti[i].board = Board();
	}
}
