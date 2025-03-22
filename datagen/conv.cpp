#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "chess.hpp"

using namespace std;
using namespace chess;

ofstream outFile("data.bullet.txt");

string result = "";
int positions = 0;
Board cur("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

class Visitor : public pgn::Visitor {
public:
	virtual ~Visitor() {}

	void startPgn() {
		// Called at the start of each PGN game
		if (!outFile.is_open()) {
			cerr << "Output file stream died" << endl;
			abort();
		}
		result = "";
		cur.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}

	void header(std::string_view key, std::string_view value) {
		// Called for each header tag (e.g., [Event "F/S Return Match"])
		if (key == "Result") {
			if (value == "1-0") result = "1.0";
			else if (value == "0-1") result = "0.0";
			else if (value == "1/2-1/2") result = "0.5";
			else {
				cerr << "Il faut que tu meures" << endl;
				abort();
			}
		}
	}

	void startMoves() {
		// Called before the moves of a game are processed
		if (result.empty()) {
			cerr << "Result is empty" << endl;
			abort();
		}
		if (cur.getFen() != "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
			cerr << "FEN is not the starting position" << endl;
			abort();
		}
	}

	void move(std::string_view move, std::string_view comment) {
		// Called for each move in the game
		Move m = uci::uciToMove(cur, (string)move);
		cur.makeMove(m);
		if (comment == "book") return;
		string commentstr = (string)comment;
		// eval is the first part of the comment up to /
		string eval = commentstr.substr(0, commentstr.find("/"));
		if (eval[1] == 'M') return; // Mate score, ignore
		float evalf = stof(eval) * 100;
		int trunc = (int)evalf;
		if (cur.sideToMove() == Color::WHITE) trunc *= -1;
		string fen = cur.getFen();
		// Ignore everything after the side to move
		string shortfen = fen.substr(0, fen.find(" ")) + " " + fen.substr(fen.find(" ") + 1, 1);

		outFile << shortfen << " | " << trunc << " | " << result << "\n";
		positions++;
	}

	void endPgn() {
		// Called at the end of each PGN game
	}
};

int main() {
	ifstream pgnFile("../fastchess/data.pgn");
	
	if (!pgnFile.is_open() || !outFile.is_open()) {
		cerr << "Error opening file." << endl;
		return 1;
	}
	
	Visitor v;
	pgn::StreamParser parser(pgnFile);
	auto err = parser.readGames(v);
	if (err) {
		cerr << "Error parsing PGN: " << err.message() << endl;
		return 1;
	}

	cout << "Converted " << positions << " positions" << endl;
}
