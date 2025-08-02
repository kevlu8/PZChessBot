#include "includes.hpp"

#include <sstream>
#include <thread>
#include <random>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

BoardState bs;

int TT_SIZE = DEFAULT_TT_SIZE;

int main(int argc, char *argv[]) {
	if (argc == 2 && std::string(argv[1]) == "bench") {
		const std::pair<std::string, int> bench_positions[] = {
			{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 12},
			{"rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6", 15},
			{"2r1r3/1b1Q1p1k/pb4q1/6p1/8/P1N3NP/2P2PP1/R3R1K1 b - - 0 29", 15},
			{"k7/8/8/8/8/8/P7/K7 w - - 0 1", 20},
			{"k7/8/8/8/8/8/8/K6R w - - 0 1", 20},
		};
		Board board = Board(TT_SIZE);
		init_network();
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
		init_network();
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
	bool online = argc == 2 && std::string(argv[1]) == "--online";
	std::cout << "PZChessBot " << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	Board board = Board(TT_SIZE);
	init_network();
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot " << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl; // Not implemented yet
			std::cout << "uciok" << std::endl;
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
				res = search(board);
			else if (depth != -1)
				res = search_depth(board, depth);
			else if (nodes != -1)
				res = search_nodes(board, nodes);
			else if (movetime != -1)
				res = search(board, movetime);
			else
				res = search(board, timemgmt(timeleft, inc, online));
			std::cout << "bestmove " << res.first.to_string() << std::endl;
		}
	}
}
