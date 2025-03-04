#include "includes.hpp"

#include <sstream>
#include <thread>

#define UCI

#include "bitboard.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "search.hpp"

int timetonodes(int remtime) {
	// Note: These values are calibrated with 10M nodes per second
	if (remtime > 20*60*1000) return 25'000'000;
	if (remtime > 3*60*1000) return 12'000'000;
	if (remtime > 60*1000) return 6'000'000;
	if (remtime > 15*1000) return 1'000'000;
	return 100'000;
}

int main() {
	std::cout << "PZChessBot v" << VERSION << " developed by kevlu8 and wdotmathree" << std::endl;
	std::string command;
	Board board = Board();
	std::thread searchthread;
	while (getline(std::cin, command)) {
		if (command == "uci") {
			std::cout << "id name PZChessBot v" << VERSION << std::endl;
			std::cout << "id author kevlu8 and wdotmathree" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			// Do nothing
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
				board.commit();
				while (ss >> move) {
					board.make_move(Move::from_string(move, &board));
					board.commit();
				}
			}
		} else if (command == "quit") {
			break;
		} else if (command == "stop") {
			// stop the search thread
			// if (searchthread.joinable()) {
			// 	searchthread.join();
			// }
		} else if (command.substr(0, 2) == "go") {
			// `go wtime ... btime ... winc ... binc ...`
			// only care about wtime and btime
			std::stringstream ss(command);
			std::string token;
			int wtime = 0, btime = 0;
			ss >> token >> token;
			if (token == "wtime") {
				ss >> wtime;
			} else if (token == "btime") {
				ss >> btime;
			}
			int timeleft = board.side ? btime : wtime;
			// start thread
			// searchthread = std::thread([timeleft, &board]() {
			// 	auto res = search(board, timetonodes(timeleft));
			// 	std::cout << "bestmove " << res.first.to_string() << std::endl;
			// });
			auto res = search(board, timetonodes(timeleft));
			std::cout << "bestmove " << res.first.to_string() << std::endl;
		}
	}
}