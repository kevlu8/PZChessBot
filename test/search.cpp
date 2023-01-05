#include "../engine/search.hpp"

std::vector<std::pair<std::string, std::string>> tests = {
	{"r2q1b1r/3k1ppp/4p3/3pPN2/3n4/5N2/3B1PPP/R2QK2R b KQ - 0 16\n5", "a8a1"}, // fork
	{"4qrk1/rb4b1/p1npp3/2p3R1/1p2P2Q/P2P3P/1PP3P1/R1B3K1 w - - 3 20\n5", "g5g7"}, // skewer
	{"r4r2/pp2Q1nk/2p4p/3p4/3p4/1B1P4/PPPK4/R5Bq w - - 2 27\n5", "e7g7"}, // double attack
	{"R7/5Npk/7p/1n1p1K1P/p2Pr1P1/P7/8/8 b - - 0 45\n2", "b5d4"}, // M1
	{"r2qkr2/1b1pn1b1/p5pp/3N4/2B5/5Q2/PPP3PP/R4RK1 w q - 2 20\n4", "f3f8"}, // M2
	{"4rr1k/pp2Nppp/8/8/2q5/2N3P1/PPP2PP1/1K1R3R w - - 1 22\n6", "h1h7"}, // Anastasia M3
	{"6k1/4Q1pp/2b1p3/2P1P3/3p1pPn/5P1P/1r1N1K2/R7 b - - 0 29\n8", "b2d2"}, // Arabian M4
	{"4r1k1/pp3p1p/6pP/6P1/8/2N1P1Q1/PPPq1P2/K3n2R b - - 6 28\n4", "d2c1"}, // Smothered M2
	{"r3k1nr/p2p1ppp/bq2P3/1N6/1p2P3/5N2/1PPBKbPP/R2Q1B1R b kq - 1 12\n", "a6b5"}, // EP + M2
};

int main() {
	int i = 1;
	for (auto &test : tests) {
		Board *board = new Board(test.first);
		std::pair<int, uint16_t> res = ab_search(*board, 5);
		if (stringify_move(res.second) == test.second) {
			std::cout << "Passed test " << i << " - Got: " << stringify_move(res.second) << std::endl;
		} else {
			std::cout << "Failed test " << i << " - Got: " << stringify_move(res.second) << " - Expected: " << test.second << std::endl;
		}
		i++;
	}
}
