#include "includes.hpp"

#include <sstream>
#include <thread>

#define UCI

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

int main(int argc, char *argv[]) {
	bool online = argc == 2 && std::string(argv[1]) == "--online";
	std::cout << "PZChessBot v" << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	Board board = Board();
	init_network();
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot v" << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			/// TODO: reset transposition table
		} else if (command.substr(0, 8) == "position") {
			// either `position startpos` or `position fen ...`
			if (command.find("startpos") != std::string::npos) {
				board = Board();
			} else if (command.find("fen") != std::string::npos) {
				std::string fen = command.substr(command.find("fen") + 4);
				if (fen.find("moves") != std::string::npos) {
					fen = fen.substr(0, fen.find("moves"));
				}
				board = Board(fen);
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
			Value score = eval(board);
			std::cout << "eval " << score / CP_SCALE_FACTOR << std::endl;
		} else if (command.substr(0, 2) == "go") {
			// `go wtime ... btime ... winc ... binc ...`
			// only care about wtime and btime
			std::stringstream ss(command);
			std::string token;
			int wtime = 0, btime = 0, winc = 0, binc = 0;
			int depth = -1;
			bool inf = false;
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
				}
			}
			int timeleft = board.side ? btime : wtime;
			int inc = board.side ? binc : winc;
			std::pair<Move, Value> res;
			if (inf)
				res = search(board);
			else if (depth != -1)
				res = search_depth(board, depth);
			else
				res = search(board, timemgmt(timeleft, inc, online));
			std::cout << "bestmove " << res.first.to_string() << std::endl;
		}
	}
}
