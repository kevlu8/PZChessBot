#include <boost/process.hpp>
#include <csignal>
#include <iostream>
#include <unordered_map>
#include <utility>

void handler(int signum) {
	std::cout << "Child died" << std::endl;
	exit(1);
}

namespace bp = boost::process;

std::vector<std::pair<std::string, std::string>> tests = {
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n1\n", "20"},
	// {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n2\n", "400"},
	// {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n3\n", "8902"},
	// {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n4\n", "197281"},
	// {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n5\n", "4865609"},

	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n1\n", "48"},
	// {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n2\n", "2039"},
	// {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n3\n", "97862"},
	// {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n4\n", "4085603"},
	// {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n5\n", "193690690"},

	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n1\n", "14"},
	// {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n2\n", "191"},
	// {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n3\n", "2812"},
	// {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n4\n", "43238"},
	// {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n5\n", "674624"},

	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1\n1\n", "6"},
	// {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1\n2\n", "264"},
	// {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1\n3\n", "9467"},
	// {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1\n4\n", "422333"},
	// {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1\n5\n", "15833292"},

	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8\n1\n", "44"},
	// {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8\n2\n", "1486"},
	// {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8\n3\n", "62379"},
	// {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8\n4\n", "2103487"},
	// {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8\n5\n", "89941194"},

	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10\n1\n", "46"},
	// {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10\n2\n", "2079"},
	// {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10\n3\n", "89890"},
	// {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10\n4\n", "3894594"},
	// {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10\n5\n", "164075551"},
};

int main() {
	signal(SIGPIPE, handler);
	bp::ipstream StdOut;
	bp::opstream StdIn;
	bp::child MyProcess = bp::child("/home/runner/PZChessBot/engine/a.out", bp::std_out > StdOut, bp::std_in < StdIn);

	for (auto &test : tests) {
		StdIn << test.first << std::endl;
		StdIn.flush();
	}

	int i = 1;
	for (auto &test : tests) {
		std::string line;
		std::getline(StdOut, line);
		std::cout << "Test: " << i << std::endl;
		std::cout << line << std::endl;
		if (line != test.second) {
			std::cout << "Failed" << std::endl;
			return 1;
		} else {
			std::cout << "Passed" << std::endl;
		}
		i++;
	}
	return 0;
}