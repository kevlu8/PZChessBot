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
ThreadInfo ti[MAX_THREADS * 2];

void datagen(ThreadInfo &tiw, ThreadInfo &tib) {
	int id = tiw.id;
	std::fstream outfile(std::to_string(id) + "_datagen.pgn", std::ios::out | std::ios::app);
	int games = 0;
	std::mt19937_64 rng(id + std::chrono::system_clock::now().time_since_epoch().count());
	while (true) {
		clear_search_vars(tiw);
		clear_search_vars(tib);
		Board &board = tiw.board;

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

		std::string game_res = "";

		int consec_draw = 0, consec_w_win = 0, consec_b_win = 0;
		int plies = DATAGEN_NUM_RAND;
		while (true) {
			auto result = search(ti);
			Move bestmove = result.first;
			Value score = result.second;
			Value w_score = score * (board.side == WHITE ? 1 : -1);

			if (abs(score) <= 20 && plies >= 80) consec_draw++;
			else consec_draw = 0;
			
			if (score >= 600) consec_w_win++;
			else consec_w_win = 0;

			if (score <= -600) consec_b_win++;
			else consec_b_win = 0;

			if (consec_draw >= 16) {
				game_res = "1/2-1/2";
				break;
			}

			if (consec_w_win >= 8) {
				game_res = "1-0";
				break;
			}

			if (consec_b_win >= 8) {
				game_res = "0-1";
				break;
			}

			board.make_move(bestmove);
			movelist.push_back(bestmove.to_san((void*)&ti.board));
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
