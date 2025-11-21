#include "includes.hpp"

#include <cassert>
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
	int id = tiw.id / 2;
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
			int mvidx = rng() % moves.size();
			tiw.board.make_move(moves[mvidx]);
			tib.board.make_move(moves[mvidx]);
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
		auto s_eval = eval(board, tiw.bs);
		if (abs(s_eval) >= 800) continue; // ridiculous position
		auto res = search(tiw).second;
		if (abs(res) >= 600) continue; // unbalanced position

		std::string startfen = board.get_fen();

		std::vector<std::string> movelist;

		std::string game_res = "";

		int consec_draw = 0, consec_w_win = 0, consec_b_win = 0;
		int plies = DATAGEN_NUM_RAND;
		while (true) {
			ThreadInfo &ti = (board.side == WHITE) ? tiw : tib;
			auto result = search(ti);
			Move bestmove = result.first;
			Value score = result.second;
			Value w_score = score * (board.side == WHITE ? 1 : -1);

			movelist.push_back(bestmove.to_san((void*)&ti.board) + " {" + bestmove.to_string() + "} ");
			tiw.board.make_move(bestmove);
			tib.board.make_move(bestmove);

			if (abs(score) <= 20 && plies >= 80) consec_draw++;
			else consec_draw = 0;

			if (score >= 600) consec_w_win++;
			else consec_w_win = 0;

			if (score <= -600) consec_b_win++;
			else consec_b_win = 0;

			if (consec_draw >= 16 || tiw.board.threefold(0) || tiw.board.insufficient_material() || tiw.board.halfmove >= 100) {
				game_res = "1/2-1/2";
				break;
			}

			if (consec_w_win >= 8 || score >= VALUE_MATE_MAX_PLY) {
				game_res = "1-0";
				break;
			}

			if (consec_b_win >= 8 || score <= -VALUE_MATE_MAX_PLY) {
				game_res = "0-1";
				break;
			}
		}
		games++;

		outfile << "[Event \"Datagen\"]\n";
		outfile << "[White \"pzchessbot-w\"]\n";
		outfile << "[Black \"pzchessbot-b\"]\n";
		outfile << "[Result \"" << game_res << "\"]\n";
		outfile << "[FEN \"" << startfen << "\"]\n";
		outfile << "\n";

		for (int i = 0; i < movelist.size(); i++) {
			if (i % 2 == 0) {
				outfile << (i / 2 + 1) << ". ";
			}
			outfile << movelist[i] << " ";
		}
		outfile << "\n\n";
		outfile.flush();

		std::cout << "Completed game " << games << std::endl;
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

	// initialize boardstates
	for (int i = 0; i < n_threads; i++) {
		for (int j = 0; j < NINPUTS * 2; j++) {
			for (int k = 0; k < NINPUTS * 2; k++) {
				for (int l = 0; l < HL_SIZE; l++) {
					bs[i][j][k].w_acc.val[l] = nnue_network.accumulator_biases[l];
					bs[i][j][k].b_acc.val[l] = nnue_network.accumulator_biases[l];
				}
				for (int l = 0; l < 64; l++) bs[i][j][k].mailbox[l] = NO_PIECE;
			}
		}
	}

	for (int i = 0; i < n_threads * 2; i++) {
		ti[i].id = i / 2;
		ti[i].bs = (void*)&bs[i / 2];
		ti[i].board = Board();
	}

	// test for now
	datagen(ti[0], ti[1]);
}
