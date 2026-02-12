#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "chess.hpp"
using namespace chess;

std::ofstream outfile("data.bullet");

class visitor : public pgn::Visitor {
private:
	Board pos;
	std::string res;
public:
	virtual ~visitor() {}

	void startPgn() {
		pos = Board();
		res = "";
	}

	void header(std::string_view key, std::string_view value) {
		if (key == "FEN") {
			pos.setFen(value);
		} else if (key == "Result") {
			res = std::string(value);
		}
	}

	void startMoves() {

	}

	void move(std::string_view move, std::string_view comment) {
		if (move == "") return;
		std::string move_str(move);
		Move m = uci::parseSan(pos, move_str);
		std::string fen = pos.getFen();
		std::string result = res == "1-0" ? "1" : res == "0-1" ? "0" : "0.5";

		bool store = true;
		if (pos.at(m.to()) != Piece::NONE) {
			store = false; // do not store captures
		}

		outfile << fen << " | 0 | " << result << '\n';

		pos.makeMove(m);
	}

	void endPgn() {
		
	}
};

int main() {
	int cnt = 0;

	std::ifstream file("game.pgn");
	if (!file.is_open()) {
		std::cerr << "Could not open file: game.pgn" << std::endl;
		return 1;
	}
	visitor v;

	pgn::StreamParser parser(file);
	auto err = parser.readGames(v);
	if (err != pgn::StreamParserError::None) {
		std::cerr << "Error parsing file: game.pgn Error code: " << err.message() << std::endl;
		return 1;
	}

	file.close();
}