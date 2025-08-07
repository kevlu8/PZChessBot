#include "includes.hpp"

#include <random>
#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

BoardState bs;

// Options
int TT_SIZE = DEFAULT_TT_SIZE;
bool quiet = false, online = false;

void run_uci() {
	std::string command;
	Board board = Board(TT_SIZE);
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl; // Not implemented yet
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
			}
		} else if (command == "ucinewgame") {
			board = Board(TT_SIZE);
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
			break;
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
			if (inf)
				res = search(board, 1e9, quiet);
			else if (depth != -1)
				res = search_depth(board, depth, quiet);
			else if (nodes != -1)
				res = search_nodes(board, nodes, quiet);
			else if (movetime != -1)
				res = search(board, movetime, quiet);
			else
				res = search(board, timemgmt(timeleft, inc, online), quiet);
			std::cout << "bestmove " << res.first.to_string() << std::endl;
		}
	}
}

__attribute__((weak)) int main(int argc, char *argv[]) {
	if (argc == 2 && std::string(argv[1]) == "bench") {
		const std::pair<std::string, int> bench_positions[] = {
			{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 12},
			{"rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6", 15},
			{"2r1r3/1b1Q1p1k/pb4q1/6p1/8/P1N3NP/2P2PP1/R3R1K1 b - - 0 29", 15},
			{"k7/8/8/8/8/8/P7/K7 w - - 0 1", 20},
			{"k7/8/8/8/8/8/8/K6R w - - 0 1", 20},
		};
		Board board = Board(TT_SIZE);
		uint64_t tot_nodes = 0;
		uint64_t start = clock();
		for (const auto &[fen, depth] : bench_positions) {
			board.reset(fen);
			search_depth(board, depth);
			tot_nodes += nodes;
		}
		uint64_t end = clock();
		std::cout << tot_nodes << " nodes " << (tot_nodes / ((double)(end - start) / CLOCKS_PER_SEC)) << " nps" << std::endl;
		return 0;
	}
	if (argc == 3 && std::string(argv[2]) == "quit") {
		// assume genfens
		// ./pzchessbot "genfens N seed S book None" "quit"
		std::string genfens = argv[1];
		std::stringstream ss(genfens);
		std::string token;
		uint64_t n = 0, s = 0;
		while (ss >> token) {
			if (token == "genfens")
				ss >> n;
			else if (token == "seed")
				ss >> s;
			else if (token == "book")
				ss >> token; // ignore book for now
		}
		Board board = Board(TT_SIZE);
		std::mt19937 rng(s);
		while (n--) {
			board.reset_startpos();
			bool restart = false;
			for (int i = 0; i < 10; i++) {
				pzstd::vector<Move> moves;
				board.legal_moves(moves);
				if (moves.size() == 0) {
					restart = true;
					break;
				}
				board.make_move(moves[rng() % moves.size()]);
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
	Board board = Board(TT_SIZE);
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl; // Not implemented yet
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

					Value display_score = score[i] / CP_SCALE_FACTOR * (board.side == BLACK ? -1 : 1);

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
						  << (score[nbucket] / CP_SCALE_FACTOR * (board.side == BLACK ? -1 : 1) > 0 ? GREEN : RED)
						  << std::showpos << (score[nbucket] / CP_SCALE_FACTOR * (board.side == BLACK ? -1 : 1)) 
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
				auto res = search(board, ms, 2); // Use quiet level 2 for pretty output
				std::cout << "Best move: " << res.first.to_string() << " with score " << res.second / CP_SCALE_FACTOR * (board.side == BLACK ? -1 : 1) << std::endl;
				board.make_move(res.first);
			} else if (command == "reset") {
				board = Board(TT_SIZE);
				std::cout << "Done" << std::endl;
			} else if (command.substr(0, 3) == "fen") {
				board = Board(TT_SIZE);
				std::string fen = command.substr(4);
				board.reset(fen);
				std::cout << "Done" << std::endl;
			} else {
				std::cout << "Unknown command: " << command << std::endl;
			}
		}
	}
}
