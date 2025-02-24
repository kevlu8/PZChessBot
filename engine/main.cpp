#include "bitboard.hpp"
#include "movegen.hpp"
#include "search.hpp"

struct MagicEntry {
	Bitboard mask;
	uint32_t offset;
};

extern Bitboard bishop_movetable[5248];
extern MagicEntry bishop_magics[64];

int main() {
	// srand(time(NULL));
	// std::string fen = "r1bqk1nr/pppp1ppp/2n4B/4p3/1bP1P3/2NP1P2/PP4PP/R2QKBNR b KQk - 2 8";
	// std::string fen = "1rb1k1nB/pppp1p2/8/n6p/2PPPqPP/8/P1K5/R2Q1BNR b - - 4 18";
	// std::string fen = "1rb1k1nB/pppp1p2/7q/4P2p/2BP2PP/8/P1K5/R2Q2NR b - - 0 20";
	// std::string fen = "1r2k1nq/ppp2p2/8/4P3/P2P2bP/5B2/2K5/R5NR b - - 1 24";
	// std::string fen = "1r2k2q/ppp2p2/4Pn2/3P4/P5BP/8/2K1N3/5R1R b - - 2 29";
	// std::string fen = "3rk3/R7/4Pn2/8/P6P/8/6q1/3BKR2 b - - 1 40";
	// std::string fen = "8/P7/1k6/8/8/8/7K/q7 b - - 4 58";

	// std::string fen = "4qrk1/rb4b1/p1npp3/2p3R1/1p2P2Q/P2P3P/1PP3P1/R1B3K1 w - - 3 20";
	// std::string fen = "4qr2/rb4k1/p1npp3/2p5/1p2P2Q/P2P3P/1PP3P1/R1B3K1 w - - 0 21";
	// std::string fen = "rnbqk1nr/pp1p1ppp/3b4/2pP4/8/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 3 7";
	Board board;
	// Board board(fen);
	while (true) {
		board.print_board();
		std::string move;
		std::cin >> move;
		if (move == "q") break;
		Move m = Move::from_string(move, &board);
		board.make_move(m);
		auto enginemove = search(board, 6);
		board.make_move(enginemove.first);
		std::cout << "Engine move: " << enginemove.first.to_string() << " Eval: " << enginemove.second << std::endl;
		// enginemove = search(board, 6);
		// std::cout << "Engine response: " << enginemove.first.to_string() << " Eval: " << enginemove.second << std::endl;
		// std::vector<Move> moves;
		// board.legal_moves(moves);
		// for (Move m : moves)
		// 	std::cout << m.to_string() << std::endl;
		// break;
	}
}
