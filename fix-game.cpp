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
		std::string fen = pos.getFen();
		std::string result = res == "1-0" ? "1" : res == "0-1" ? "0" : "0.5";
		outfile << fen << " | 0 | " << result << '\n';

		Move m = uci::parseSan(pos, move_str);
		pos.makeMove(m);
	}

	void endPgn() {
		
	}
};

int main() {
	const std::string path = ".";

	int cnt = 0;

	// list all pgn files in the data directory
	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		if (entry.path().extension() == ".pgn") {
			std::string file_path = entry.path().string();
			std::ifstream file(file_path);
			if (!file.is_open()) {
				std::cerr << "Could not open file: " << file_path << std::endl;
				continue;
			}
			visitor v;

			pgn::StreamParser parser(file);
			auto err = parser.readGames(v);
			if (err != pgn::StreamParserError::None) {
				std::cerr << "Error parsing file: " << file_path << " Error code: " << err.message() << std::endl;
				continue;
			}

			file.close();
		}
	}
}