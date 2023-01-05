#include <boost/process.hpp>
#include <csignal>
#include <iostream>
#include <math.h>
#include <unordered_map>
#include <utility>

void handler(int signum) {
	std::cout << "Child died" << std::endl;
	exit(1);
}

namespace bp = boost::process;

bool failed = false;

std::vector<std::pair<std::string, std::string>> tests = {
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "20"},
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "400"},
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "8902"},
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "197281"},
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "4865609"},

	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "48"},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "2039"},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "97862"},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "4085603"},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "193690690"},

	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "14"},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "191"},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "2812"},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "43238"},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "674624"},

	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "6"},
	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "264"},
	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "9467"},
	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "422333"},
	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "15833292"},

	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "44"},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "1486"},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "62379"},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "2103487"},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "89941194"},

	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "46"},
	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "2079"},
	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "89890"},
	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "3894594"},
	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "164075551"},

	{"quit", "quit"},
};

int main() {
	signal(SIGPIPE, handler);
	bp::ipstream StdOut;
	bp::opstream StdIn;
	bp::child MyProcess = bp::child("./engine/a.out", bp::std_out > StdOut, bp::std_in < StdIn);

	int i = 1, j = 1;
	for (auto &test : tests) {
		StdIn << test.first << std::endl;
		StdIn.flush();
		StdIn << std::to_string(j) << std::endl;
		StdIn.flush();
		j++;
		if (j == 6)
			j = 1;
	}
	std::string line;
	std::pair<std::string, std::string> test;
	for (int i = 1; i < tests.size(); i++) {
		std::getline(StdOut, line);
		test = tests[i - 1];
		if (line != test.second) {
			std::cout << "Failed test " << ceil((float)i / 5) << '.' << (((i % 5) == 0) ? 5 : (i % 5)) << " - ";
			std::cout << "Got: " << line << " - ";
			std::cout << "Expected: " << test.second << std::endl;
			failed = true;
		} else {
			std::cout << "Passed test " << ceil((float)i / 5) << '.' << (((i % 5) == 0) ? 5 : (i % 5)) << " - ";
			std::cout << "Got: " << line << std::endl;
		}
		if (i % 5 == 0)
			std::cout << std::endl;
	}
	return failed;
}