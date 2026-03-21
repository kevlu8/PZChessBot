#include "includes.hpp"

#include <atomic>
#include <chrono>
#include <random>
#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "eval.hpp"
#include "history.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"
#include "ttable.hpp"

#include "dfrc.hpp"

#define MAX_TT (262144)

// Options
size_t TT_SIZE = DEFAULT_TT_SIZE;
bool quiet = false, online = false, dfrc_uci = false;

ThreadInfo *tis;

std::atomic<uint64_t> g_tot_pos(0), g_tot_games(0), threads_done(0);
int n_threads = 1;

uint16_t move_to_bin(Move move) {
	uint16_t data = 0;
	data = move.src() | (move.dst() << 6);
	if (move.type() == PROMOTION) {
		PieceType pt = PieceType(move.promotion() + KNIGHT);
		if (pt == BISHOP) data |= 1 << 12;
		else if (pt == ROOK) data |= 2 << 12;
		else if (pt == QUEEN) data |= 3 << 12;
	}
	if (move.type() == EN_PASSANT) data |= 1 << 14;
	else if (move.type() == CASTLING) data |= 2 << 14;
	else if (move.type() == PROMOTION) data |= 3 << 14;
	return data;
}

void datagen(ThreadInfo &tiw, ThreadInfo &tib) {
	int id = tiw.id / 2;

	std::string filename = std::to_string(id) + "_datagen.viri";

	char header_buf[1048576];
	int header_ptr = 0;
	char move_buf[1048576];
	int ptr = 0;

	std::fstream outfile(filename, std::ios::out | std::ios::binary);
	uint64_t games = 0;
	std::mt19937_64 rng(id + std::chrono::system_clock::now().time_since_epoch().count());
	while (threads_done.load() != n_threads && games < DATAGEN_MAX_GAMES) {
		bool do_adjudication = (rng() % 100) < DATAGEN_ADJUDICATION_PERCENT;
		bool dfrc = (rng() % 100) < DATAGEN_DFRC_PERCENT;
		clear_search_vars(tiw);
		clear_search_vars(tib);
		Position &pos = tiw.pos;
		RepetitionHandler &rp = tiw.rp;

		header_ptr = ptr = 0;

		std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

		if (dfrc) {
			std::string w_home = frcFens[rng() % 960];
			for (auto &c : w_home) c = std::toupper(c);
			std::string b_home = frcFens[rng() % 960];
			start_fen = b_home + "/pppppppp/8/8/8/8/PPPPPPPP/" + w_home + " w KQkq - 0 1";
		}

		tiw.pos.reset(start_fen);
		tib.pos.reset(start_fen);
		tiw.rp.clear();
		tib.rp.clear();
		tiw.rp.push_hash(tiw.pos.zobrist);
		tib.rp.push_hash(tib.pos.zobrist);

		auto write_buf = [&](char *data, int sz) {
			memcpy(move_buf + ptr, data, sz);
			ptr += sz;
		};

		auto write_header_buf = [&](char *data, int sz) {
			memcpy(header_buf + header_ptr, data, sz);
			header_ptr += sz;
		};

		// write viriformat header
		// 1. occupancy
		Bitboard occ = pos.piece_boards[OCC(WHITE)] | pos.piece_boards[OCC(BLACK)];
		write_header_buf((char *)&occ, 8);
		// 2. piece info
		std::vector<uint8_t> pieces;
		auto castles = pos.castling;
		bool seen_white_king = false, seen_black_king = false;
		for (int sq = 0; sq < 64; sq++) {
			Piece p = pos.mailbox[sq];
			if (p == NO_PIECE) continue;

			PieceType pt = PieceType(p & 7);
			uint8_t piece_info = 0;
			if (p & 8) piece_info |= 0b1000; // black pieces
			if (pt == KNIGHT) piece_info |= 1;
			else if (pt == BISHOP) piece_info |= 2;
			else if (pt == QUEEN) piece_info |= 4;
			else if (pt == KING) piece_info |= 5;
			// pawn is 0

			if (pt == KING) {
				if (p & 8) seen_black_king = true;
				else seen_white_king = true;
			}

			if (pt == ROOK) {
				if (!seen_white_king && (castles & WHITE_OOO)) piece_info |= 6;
				else if (seen_white_king && (castles & WHITE_OO)) piece_info |= 6;
				else if (!seen_black_king && (castles & BLACK_OOO)) piece_info |= 6;
				else if (seen_black_king && (castles & BLACK_OO)) piece_info |= 6;
				else piece_info |= 3; // normal rook
			}

			pieces.push_back(piece_info);
		}
		while (pieces.size() < 32) pieces.push_back(0);
		for (int i = 0; i < 32; i += 2) {
			uint8_t byte = (pieces[i] << 4) | pieces[i + 1];
			write_header_buf((char *)&byte, 1);
		}

		// 3. stm + EP
		uint8_t meta = 0;
		if (pos.side == BLACK) meta |= 0b10000000;
		// EP technically should be included but startpos cannot have EP
		meta |= 64;
		write_header_buf((char *)&meta, 1);

		// 4. hm clock
		uint8_t hm = pos.halfmove;
		write_header_buf((char *)&hm, 1);

		// 5. fm clock
		uint16_t fm = 1;
		write_header_buf((char *)&fm, 2);

		// 6. eval of cur pos (0)
		int16_t eval_ = 0;
		write_header_buf((char *)&eval_, 2);

		// 7. game result (pending - 0 = black, 1 = draw, 2 = white)

		for (int i = 0; i < DATAGEN_NUM_RAND; i++) {
			pzstd::vector<Move> moves;
			pos.legal_moves(moves);
			if (moves.size() == 0) break;
			int mvidx = rng() % moves.size();
			Move mv = moves[mvidx];
			tiw.pos.make_move(mv);
			tib.pos.make_move(mv);

			uint16_t binmove = move_to_bin(mv);
			write_buf((char *)&binmove, 2);
			uint16_t eval_ = 0;
			write_buf((char *)&eval_, 2);
		}

		if (_mm_popcnt_u64(pos.piece_boards[KING]) != 2) continue;

		bool in_check = false, checking_opponent = false;
		in_check = pos.checkers[pos.side];
		checking_opponent = pos.checkers[!pos.side];
		if (in_check || checking_opponent) continue;

		auto s_eval = eval(pos, tiw.am);
		if (abs(s_eval) >= 800) continue; // ridiculous position
		auto res = search(tiw.pos, tiw.rp, &tiw).second;
		if (abs(res) >= 600) continue; // unbalanced position

		std::string startfen = pos.get_fen();

		std::vector<std::string> movelist;

		uint8_t game_res = 0;

		int consec_draw = 0, consec_w_win = 0, consec_b_win = 0;
		int plies = 0;
		while (true) {
			tiw.nodes = tib.nodes = 0;
			ThreadInfo &ti = (pos.side == WHITE) ? tiw : tib;
			auto result = search(ti.pos, ti.rp, &ti);
			Move bestmove = result.first;
			Value score = result.second;
			Value w_score = score * (pos.side == WHITE ? 1 : -1);

			if (score == VALUE_STALE) {
				game_res = 1;
				break;
			}

			tiw.pos.make_move(bestmove);
			tib.pos.make_move(bestmove);

			if (abs(score) <= 20 && plies >= 100) consec_draw++;
			else consec_draw = 0;

			if (w_score >= 2000) consec_w_win++;
			else consec_w_win = 0;

			if (w_score <= -2000) consec_b_win++;
			else consec_b_win = 0;

			if ((consec_draw >= 16 && do_adjudication) || tiw.rp.threefold(0, tiw.pos.zobrist_without_ep()) || tiw.pos.insufficient_material() || tiw.pos.halfmove >= 100) {
				game_res = 1;
				break;
			}

			if ((consec_w_win >= 8 && do_adjudication) || w_score >= VALUE_MATE_MAX_PLY) {
				game_res = 2;
				break;
			}

			if ((consec_b_win >= 8 && do_adjudication) || w_score <= -VALUE_MATE_MAX_PLY) {
				game_res = 0;
				break;
			}

			uint16_t binmove = move_to_bin(bestmove);
			write_buf((char *)&binmove, 2);
			write_buf((char *)&w_score, 2);

			plies++;
		}
		games++;

		// 7. game result
		write_header_buf((char *)&game_res, 1);
		
		// 8. padding
		uint32_t pad = 0;
		write_header_buf((char *)&pad, 1);
		write_buf((char *)&pad, 4);

		// flush header buf into file
		outfile.write(header_buf, header_ptr);
		header_ptr = 0;

		// flush buf into file
		outfile.write(move_buf, ptr);
		ptr = 0;

		g_tot_pos += plies;
		g_tot_games += 1;
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
	if (argc >= 2) {
		n_threads = std::stoi(argv[1]);
		if (n_threads < 1 || n_threads > MAX_THREADS) {
			std::cerr << "Invalid thread count" << std::endl;
			return 1;
		}
	}
	tis = new ThreadInfo[n_threads * 2];
	for (int i = 0; i < n_threads * 2; i++) {
		tis[i].id = i;
		tis[i].pos = Position();
		tis[i].rp = RepetitionHandler();
		tis[i].rp.push_hash(tis[i].pos.zobrist);
	}
	std::cout << "PZChessBot " << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::cout << "Using " << n_threads << " threads for datagen" << std::endl;

	std::vector<std::thread> threads;
	for (int i = 0; i < n_threads; i++) {
		threads.emplace_back(datagen, std::ref(tis[i * 2]), std::ref(tis[i * 2 + 1]));
	} 

	std::thread rep_thread(reporter);

	while (threads_done.load() != n_threads) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	for (auto &t : threads) {
		t.join();
	}
	rep_thread.join();

	std::cout << "Done. Total positions: " << g_tot_pos.load() << std::endl;
}
