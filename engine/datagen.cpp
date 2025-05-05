#include "includes.hpp"

#include <atomic>
#include <sstream>
#include <thread>
#include <vector>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

std::vector<std::string> book;
int book_idx = 0;

int main(int argc, char *argv[]) {
    // ./a.out [NPOS] [NODES] [OUTPUT_FILE] [SEED] [BOOK-FILE]
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " [NPOS] [NODES] [OUTPUT_FILE] [SEED] [BOOK-FILE (opt)]" << std::endl;
        return 1;
    }
    if (argc == 6) {
        // book file specified
        std::ifstream book_file(argv[5]);
        std::string line;
        while (std::getline(book_file, line)) {
            book.push_back(line); // all contains fens
        }
    }
    int NPOS = std::stoi(argv[1]);
    int FIXED_NODES = std::stoi(argv[2]);
    std::string OUT_FILE = argv[3];
    int SEED = std::stoi(argv[4]);
    // Open file w/ append
    std::ofstream outfile(OUT_FILE, std::ios::app);
    std::cout << "PZChessBot " << VERSION << " data generation script" << std::endl;
    srand(SEED);

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
        if (book_idx >= book.size()) {
            bool restart = false;
            for (int i = 0; i < 10; i++) {
                pzstd::vector<Move> moves;
                board.legal_moves(moves);
                if (moves.size() == 0) {
                    restart = true;
                    break;
                }
                board.make_move(moves[rand() % moves.size()]);
            }
            if (_mm_popcnt_u64(board.piece_boards[KING]) != 2) {
                // If somehow a side is missing a king, restart
                restart = true;
            }
            if (restart) continue;
        } else {
            board = Board(book[book_idx++]);
        }
        // Self play time!
        Value eval = 0;
        std::vector<std::pair<std::string, Value>> game; // fen, eval
        std::string res = "";
        int plies = 0;
        while (abs(eval) < VALUE_MATE_MAX_PLY) {
            if (board.threefold() || board.halfmove >= 100 || (plies >= 100 && abs(eval) < 100) || plies >= 200) {
                // Probably drawn, stop the game
                res = "0.5";
                break;
            }
            // Search for a move
            std::pair<Move, Value> result = search_nodes(board, FIXED_NODES);
            // std::pair<Move, Value> result = search_depth(board, 7);
            // Get the eval
            eval = result.second;
			Value white_centric_eval = board.side == WHITE ? eval : -eval;
            if (white_centric_eval >= VALUE_MATE_MAX_PLY) {
                res = "1.0";
                break;
            } else if (white_centric_eval <= -VALUE_MATE_MAX_PLY) {
                res = "0.0";
                break;
            }
            
            bool capture = result.first.dst() & board.piece_boards[OPPOCC(board.side)];

            plies++;

            if (!capture) game.push_back({board.get_fen(), white_centric_eval});
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
            std::cout << "Time to end: " << ((double)(clock() - start) / CLOCKS_PER_SEC) * (NPOS - positions) / (positions+1) << "s" << std::endl;
        }
    }
	std::cout << "Finished." << std::endl;
	std::cout << "Generated " << positions << " positions in " << games << " games in " << (clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
	std::cout << "Positions / second: " << (positions / ((double)(clock() - start) / CLOCKS_PER_SEC)) << std::endl;
	outfile.close();
	return 0;
}