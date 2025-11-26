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

// Options
int TT_SIZE = DEFAULT_TT_SIZE;
bool quiet = false, online = false;

ThreadInfo *tis;

void run_uci() {
	std::string command;
	Board board = Board();
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 64" << std::endl;
			std::cout << "option name Quiet type check default false" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if (command == "icu") {
			return; // exit uci mode
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command.substr(0, 9) == "setoption") {
			std::string optionname, optionvalue, token;
			std::stringstream ss(command);
			ss >> token;
			while (ss >> token) {
				if (token == "name") {
					ss >> optionname;
				} else if (token == "value") {
					ss >> optionvalue;
				}
			}
			if (optionname == "Hash") {
				int optionint = std::stoi(optionvalue);
				if (optionint < 1 || optionint > 1024) {
					std::cerr << "Invalid hash size: " << optionint << std::endl;
					continue;
				}
				TT_SIZE = optionint * 1024 * 1024 / sizeof(TTable::TTBucket);
			} else if (optionname == "Quiet") {
				quiet = optionvalue == "true";
			} else if (optionname == "Threads") {
				num_threads = std::stoi(optionvalue);
				if (num_threads < 1 || num_threads > 64) {
					std::cerr << "Invalid number of threads: " << num_threads << std::endl;
					num_threads = 1;
				}
				delete[] tis;
				tis = new ThreadInfo[num_threads];
				for (int i = 0; i < num_threads; i++) tis[i].set_bs();
				std::cout << "info string Using " << num_threads << " threads" << std::endl;
			}
		} else if (command == "ucinewgame") {
			board = Board();
			ttable.resize(TT_SIZE);
			for (int i = 0; i < num_threads; i++) {
				clear_search_vars(tis[i]);
			}
		} else if (command.substr(0, 8) == "position") {
			// either `position startpos` or `position fen ...`
			if (command.find("startpos") != std::string::npos) {
				board.reset_startpos();
			} else if (command.find("fen") != std::string::npos) {
				std::string fen = command.substr(command.find("fen") + 4);
				if (fen.find("moves") != std::string::npos) {
					fen = fen.substr(0, fen.find("moves"));
				}
				board.reset(fen);
			}
			if (command.find("moves") != std::string::npos) {
				std::string moves = command.substr(command.find("moves") + 6);
				std::stringstream ss(moves);
				std::string move;
				while (ss >> move) {
					board.make_move(Move::from_string(move, &board));
				}
			}
		} else if (command == "quit") {
			exit(0);
		} else if (command == "stop") {
			// stop the search thread
			// if (searchthread.joinable()) {
			// 	searchthread.join();
			// }
		} else if (command == "eval") {
			std::array<Value, 8> score = debug_eval(board);
			board.print_board();
			std::cout << "info string fen " << board.get_fen() << std::endl;
			int nbucket = (_mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) - 2) / 4;
			for (int i = 0; i < 8; i++) {
				std::cout << "info string eval " << i << ": " << score[i];
				if (i == nbucket) {
					std::cout << " (current)";
				}
				std::cout << std::endl;
			}
		} else if (command.substr(0, 2) == "go") {
#ifndef HCE
			std::cout << "info string Using " << NNUE_PATH << " for evaluation" << std::endl;
#endif
			// `go wtime ... btime ... winc ... binc ...`
			// only care about wtime and btime
			std::stringstream ss(command);
			std::string token;
			int wtime = 0, btime = 0, winc = 0, binc = 0;
			int depth = -1;
			int nodes = -1;
			bool inf = false;
			int movetime = -1;
			ss >> token;
			while (ss >> token) {
				if (token == "wtime") {
					ss >> wtime;
				} else if (token == "btime") {
					ss >> btime;
				} else if (token == "winc") {
					ss >> winc;
				} else if (token == "binc") {
					ss >> binc;
				} else if (token == "depth") {
					ss >> depth;
				} else if (token == "infinite") {
					inf = true;
				} else if (token == "nodes") {
					ss >> nodes;
				} else if (token == "movetime") {
					ss >> movetime;
				}
			}
			int timeleft = board.side ? btime : wtime;
			int inc = board.side ? binc : winc;
			std::pair<Move, Value> res;
			if (inf) res = search(board, tis, 1e18, MAX_PLY, 1e18, quiet);
			else if (depth != -1) res = search(board, tis, 1e18, depth, 1e18, quiet);
			else if (nodes != -1) res = search(board, tis, 1e18, MAX_PLY, nodes, quiet);
			else if (movetime != -1) res = search(board, tis, movetime, MAX_PLY, 1e18, quiet);
			else res = search(board, tis, timemgmt(timeleft, inc, online), MAX_PLY, 1e18, quiet);
			std::cout << "bestmove " << res.first.to_string() << std::endl;
		}
	}
}

__attribute__((weak)) int main(int argc, char *argv[]) {
	tis = new ThreadInfo[1]; // single thread for now
	tis[0].set_bs();
	if (argc == 2 && std::string(argv[1]) == "bench") {
		const std::string bench_positions[] = {
			"r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq - 0 14",
            "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
            "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
            "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
            "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
            "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
            "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
            "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
            "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
            "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
            "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
            "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
            "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq - 0 17",
            "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
            "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
            "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
            "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
            "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
            "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
            "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
            "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
            "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
            "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
            "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
            "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
            "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
            "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
            "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
            "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
            "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
            "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
            "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
            "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
            "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
            "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
            "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
            "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
            "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
            "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
            "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
            "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
            "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
            "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
            "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
            "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
            "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
            "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq - 0 2",
            "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
            "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
            "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93",
		};
		Board board = Board();
		uint64_t tot_nodes = 0;
		uint64_t start = clock();
		for (const auto &fen : bench_positions) {
			board.reset(fen);
			clear_search_vars(tis[0]);
			search(board, tis, 1e9, 12, 1e18, 0);
			tot_nodes += nodes[0];
		}
		uint64_t end = clock();
		std::cout << tot_nodes << " nodes " << int(tot_nodes / ((double)(end - start) / CLOCKS_PER_SEC)) << " nps" << std::endl;
		return 0;
	}
	if (argc == 3 && std::string(argv[2]) == "quit") {
		// assume genfens
		// ./pzchessbot "genfens N seed S book None" "quit"
		bool filter_weird = true;
		int nmoves = 12;
		std::string genfens = argv[1];
		std::stringstream ss(genfens);
		std::string book, token;
		uint64_t n = 0, s = 0;
		while (ss >> token) {
			if (token == "genfens")
				ss >> n;
			else if (token == "seed")
				ss >> s;
			else if (token == "book")
				ss >> book;
			else if (token == "filter") {
				ss >> token;
				filter_weird = token == "1" || token == "true";
			} else if (token == "nmoves") {
				ss >> nmoves;
			}
		}
		Board board = Board();
		std::mt19937_64 rng(s);
		std::ifstream bookfile(book == "None" ? "" : book);
		std::vector<std::string> fens;
		if (bookfile.is_open()) {
			std::string line;
			while (getline(bookfile, line)) {
				fens.push_back(line);
			}
			bookfile.clear();
			bookfile.close();
		}
		while (n--) {
			if (fens.empty()) board.reset_startpos();
			else {
				board.reset(fens[rng() % fens.size()]);
			}
			bool restart = false;
			for (int i = 0; i < nmoves; i++) {
				pzstd::vector<Move> moves;
				board.legal_moves(moves);
				if (moves.size() == 0) {
					restart = true;
					break;
				}
				board.make_move(moves[rng() % moves.size()]);
			}
			bool in_check = false, checking_opponent = false;
			if (board.side == WHITE) {
				in_check = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK);
				checking_opponent = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE);
			} else {
				in_check = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)]), WHITE);
				checking_opponent = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)]), BLACK);
			}
			if (in_check || checking_opponent) restart = true;
			// make sure position is legal and somewhat balanced
			if (!restart) {
				if (_mm_popcnt_u64(board.piece_boards[KING]) != 2) restart = true;
				else if (filter_weird) {
					int npieces = _mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]);
					auto s_eval = debug_eval(board)[(npieces - 2) / 4] * (board.side == WHITE ? 1 : -1);
					if (abs(s_eval) >= 600) restart = true; // do a fast static eval to quickly filter out crazy positions
					else {
						auto res = search(board, tis, 1e9, MAX_PLY, 10000, 1);
						if (abs(res.second) >= 400) restart = true;
					}
				}
			}
			if (restart) {
				n++;
				continue;
			}
			std::cout << "info string genfens " << board.get_fen() << std::endl;
		}
		return 0;
	}
	online = argc >= 2 && std::string(argv[1]) == "--online=1";
	std::cout << "PZChessBot " << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	Board board = Board();
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 64" << std::endl;
			std::cout << "option name Quiet type check default false" << std::endl;
			std::cout << "uciok" << std::endl;
			run_uci();
		} else if (command == "quit") {
			return 0;
		} else if (command == "help") {
			std::cout << "Available commands:" << std::endl;
			std::cout << "  uci         - Start the UCI protocol" << std::endl;
			std::cout << "  quit        - Quit the program" << std::endl;
			std::cout << "  help        - Show this help message" << std::endl;
			std::cout << "  eval        - Statically evaluate the current position" << std::endl;
			std::cout << "  d           - Display the current board position" << std::endl;
			std::cout << "  move <move> - Make a move (e.g., 'move e2e4')" << std::endl;
			std::cout << "  go <ms>     - Start the search for the best move then play it, searching for <ms> milliseconds" << std::endl;
			std::cout << "  reset       - Reset the board to the starting position" << std::endl;
			std::cout << "  fen <fen>   - Set the board position from a FEN string" << std::endl;
		} else {
			// General pretty printed commands here
			if (command == "eval") {
				std::array<Value, 8> score = debug_eval(board);

				std::cout << "\n" << CYAN "═══════════════════════════════════════" RESET << std::endl;
				std::cout << BOLD CYAN "           POSITION EVALUATION          " RESET << std::endl;
				std::cout << CYAN "═══════════════════════════════════════" RESET << std::endl;

				board.print_board_pretty();

				int nbucket = (_mm_popcnt_u64(board.piece_boards[OCC(WHITE)] | board.piece_boards[OCC(BLACK)]) - 2) / 4;

				std::cout << BOLD YELLOW "Bucketed eval:" RESET << std::endl;
				std::cout << CYAN "─────────────────────────────" RESET << std::endl;

				for (int i = 0; i < 8; i++) {
					std::string indicator = (i == nbucket) ? GREEN "→ " : "  ";
					std::string current_marker = (i == nbucket) ? BOLD GREEN " (CURRENT)" RESET : "";

					Value display_score = score[i] * (board.side == BLACK ? -1 : 1);

					std::string score_color;
					if (display_score > 100) score_color = GREEN;
					else if (display_score > 0) score_color = YELLOW;
					else if (display_score > -100) score_color = MAGENTA;
					else score_color = RED;

					std::cout << std::noshowpos << indicator << BLUE "Bucket " << i << ": " RESET
							  << score_color << std::setw(6) << std::showpos << display_score << RESET
							  << current_marker << std::endl;
				}

				std::cout << CYAN "─────────────────────────────" RESET << std::endl;
				std::cout << YELLOW "Current evaluation: " RESET
						  << (score[nbucket] * (board.side == BLACK ? -1 : 1) > 0 ? GREEN : RED)
						  << std::showpos << (score[nbucket] * (board.side == BLACK ? -1 : 1)) 
						  << " centipawns" << RESET << std::endl;
				std::cout << CYAN "═══════════════════════════════════════" RESET << std::endl << std::endl;
				std::cout << std::noshowpos;
			} else if (command == "d") {
				board.print_board_pretty(true);
			} else if (command.substr(0, 4) == "move") {
				pzstd::vector<Move> moves;
				board.legal_moves(moves);
				std::string move_str = command.substr(5);

				bool exists = false;
				for (const Move &m : moves) {
					if (m.to_string() == move_str) {
						exists = true;
						break;
					}
				}

				if (!exists) {
					std::cout << RED "Invalid move: " RESET << move_str << std::endl;
				} else {
					Move move = Move::from_string(move_str, &board);
					board.make_move(move);
					std::cout << YELLOW "Move made: " RESET << move.to_string() << std::endl;
					board.print_board_pretty();
				}
			} else if (command.substr(0, 2) == "go") {
				int ms = std::stoi(command.substr(3));
				auto res = search(board, tis, ms, MAX_PLY, 1e18, 2); // Use quiet level 2 for pretty output
				std::cout << CYAN "Best move: " RESET BOLD << res.first.to_string() << RESET
						  << CYAN " with score: " RESET << (res.second * (board.side == BLACK ? -1 : 1) > 0 ? GREEN : RED)
						  << std::showpos << res.second * (board.side == BLACK ? -1 : 1) << " cp" << RESET << std::endl << std::noshowpos;
			} else if (command == "undo") {
				board.unmake_move();
				board.print_board_pretty();
			} else if (command == "reset") {
				board = Board();
				ttable.resize(TT_SIZE);
				std::cout << "Done" << std::endl;
			} else if (command.substr(0, 3) == "fen") {
				board = Board();
				std::string fen = command.substr(4);
				board.reset(fen);
				std::cout << "Done" << std::endl;
			} else {
				std::cout << "Unknown command: " << command << std::endl;
			}
		}
	}
}
