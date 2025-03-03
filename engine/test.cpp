#include "bitboard.hpp"
#include "movegen.hpp"
#include "search.hpp"

int main() {
	// std::string fen = "1r1r2k1/pp3ppp/4bn2/8/2qNp3/P1P1P2P/1B1Q1PP1/1K1R1R2 w - - 10 22"; // next d2c2

	Board board;
	// Board board(fen);
	while (true) {
		board.print_board();
		std::string move;
		std::cin >> move;
		if (move == "q") break;
		Move m = Move::from_string(move, &board);
		board.make_move(m);
		auto enginemove = search(board, 4);
		board.make_move(enginemove.first);
		std::cout << "Engine move: " << enginemove.first.to_string() << " Eval: " << enginemove.second << std::endl;
	}
}
