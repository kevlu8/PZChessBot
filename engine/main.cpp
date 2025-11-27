#include "includes.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
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

std::atomic<uint64_t> g_tot_pos(0), g_tot_games(0), threads_done(0);
int n_threads = 1;

void datagen(ThreadInfo &tiw, ThreadInfo &tib) {
	int id = tiw.id / 2;

	std::string filename = std::to_string(id) + "_datagen.pgn";

	std::fstream outfile(filename, std::ios::out);
	uint64_t games = 0, total_pos = 0;
	std::mt19937_64 rng(id + std::chrono::system_clock::now().time_since_epoch().count());
	while (threads_done.load() != n_threads && games < DATAGEN_MAX_GAMES) {
		bool do_adjudication = (rng() % 100) < DATAGEN_ADJUDICATION_PERCENT;
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
			tiw.nodes = tib.nodes = 0;
			tiw.early_exit = tib.early_exit = false;
			ThreadInfo &ti = (board.side == WHITE) ? tiw : tib;
			auto result = search(ti);
			Move bestmove = result.first;
			Value score = result.second;
			Value w_score = score * (board.side == WHITE ? 1 : -1);

			if (score == VALUE_STALE) {
				game_res = "1/2-1/2";
				break;
			}

			std::stringstream movess;
			movess << bestmove.to_san((void *)&ti.board) << " {" << std::fixed << std::setprecision(2) << std::showpos << (score / 100.0) << "}";

			movelist.push_back(movess.str());
			tiw.board.make_move(bestmove);
			tib.board.make_move(bestmove);

			if (abs(score) <= 20 && plies >= 80) consec_draw++;
			else consec_draw = 0;

			if (w_score >= 600) consec_w_win++;
			else consec_w_win = 0;

			if (w_score <= -600) consec_b_win++;
			else consec_b_win = 0;

			if ((consec_draw >= 16 && do_adjudication) || tiw.board.threefold(0) || tiw.board.insufficient_material() || tiw.board.halfmove >= 100) {
				game_res = "1/2-1/2";
				break;
			}

			if ((consec_w_win >= 8 && do_adjudication) || w_score >= VALUE_MATE_MAX_PLY) {
				game_res = "1-0";
				break;
			}

			if ((consec_b_win >= 8 && do_adjudication) || w_score <= -VALUE_MATE_MAX_PLY) {
				game_res = "0-1";
				break;
			}

			plies++;
		}
		games++;
		total_pos += plies;

		outfile << "[Result \"" << game_res << "\"]\n";
		outfile << "[FEN \"" << startfen << "\"]\n";
		outfile << "\n";

		for (int i = 0; i < movelist.size(); i++) {
			outfile << movelist[i] << " ";
		}
		outfile << "\n\n";
		outfile.flush();

		if (games % 10 == 0) {
			g_tot_pos += total_pos;
			total_pos = 0;
			g_tot_games += 10;
		}
	}

	outfile.close();
	threads_done++;
}

void reporter() {
	auto start_time = std::chrono::high_resolution_clock::now();
	auto last_report = start_time;
	while (threads_done.load() != n_threads) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auto current_time = std::chrono::high_resolution_clock::now();
		if (last_report + std::chrono::seconds(30) > current_time) continue;
		last_report = current_time;
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
		uint64_t positions = g_tot_pos.load();
		double pps = positions / (double)elapsed;
		std::cout << "Positions: " << positions << ", Time: " << elapsed << "s, PPS: " << pps << ", Games: " << g_tot_games.load() << std::endl;
	}
}

int main(int argc, char *argv[]) {
	// ./pzchessbot [num_threads]
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
		ti[i].id = i;
		ti[i].bs = (void *)&bs[i / 2];
		ti[i].board = Board();
	}

	std::vector<std::thread> threads;
	for (int i = 0; i < n_threads; i++) {
		threads.emplace_back(datagen, std::ref(ti[i * 2]), std::ref(ti[i * 2 + 1]));
	}

	std::thread rep_thread(reporter);

	while (threads_done.load() != n_threads) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	for (auto &t : threads) {
		t.join();
	}
	rep_thread.join();

	std::cout << "Done. Total positions: " << g_tot_pos.load() << std::endl;
}
