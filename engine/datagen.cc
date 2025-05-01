#include "includes.hpp"

#include <atomic>
#include <sstream>
#include <thread>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

#define OUT_FILE "data.bullet.txt"
#define FIXED_NODES 15000
#define NPOS 1'000'000

int main(int argc, char *argv[]) {
    // Data generation script
    std::ofstream outfile(OUT_FILE);
    std::cout << "PZChessBot " << VERSION << " data generation script" << std::endl;
    srand(time(NULL));

    init_network();

    std::cout << "startpos eval: ";
    {
        Board board = Board();
        std::array<Value, 8> score = debug_eval(board);
        for (int i = 0; i < 8; i++) {
            std::cout << score[i] << " ";
        }
        std::cout << std::endl;
    }

    // Go!
    clock_t start = clock();
    int positions = 0, games = 0;
    while (positions < NPOS) {
        Board board = Board();
        for (int i = 0; i < 8; i++) {
            pzstd::vector<Move> moves;
            board.legal_moves(moves);
            board.make_move(moves[rand() % moves.size()]);
        }
        // Self play time!
        Value eval = 0;
        pzstd::largevector<std::pair<std::string, Value>> game; // fen, eval
        std::string res = "";
        while (abs(eval) < VALUE_MATE_MAX_PLY) {
            if ((game.size() >= 100 && abs(eval) < 100) || game.size() >= 400) {
                // Probably drawn, stop the game
                res = "0.5";
                break;
            }
            // Search for a move
            std::pair<Move, Value> result = search_nodes(board, FIXED_NODES);
            // Get the eval
            eval = result.second;
            if (eval >= VALUE_MATE_MAX_PLY) {
                res = "1.0";
                break;
            } else if (eval <= -VALUE_MATE_MAX_PLY) {
                res = "0.0";
                break;
            }
			Value white_centric_eval = board.side == WHITE ? eval : -eval;
            game.push_back({board.get_fen(), white_centric_eval});
            // Make the move
            board.make_move(result.first);
        }
        // Write the game to the file
        for (auto &entry : game) {
            // [fen] | [eval] | [result]
            outfile << entry.first << " | " << entry.second / CP_SCALE_FACTOR << " | " << res << std::endl;
        }
        positions += game.size();
        games++;
        if (games % 50 == 0) {
            std::cout << "Generated " << positions << " positions in " << games << " games in " << (clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
            std::cout << "Positions / second: " << (positions / ((double)(clock() - start) / CLOCKS_PER_SEC)) << " ";
            std::cout << "Time to end: " << ((double)(clock() - start) / CLOCKS_PER_SEC) * (NPOS - positions) / positions << "s" << std::endl;
        }
    }
	std::cout << "Finished." << std::endl;
	std::cout << "Generated " << positions << " positions in " << games << " games in " << (clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
	std::cout << "Positions / second: " << (positions / ((double)(clock() - start) / CLOCKS_PER_SEC)) << std::endl;
	outfile.close();
	return 0;
}