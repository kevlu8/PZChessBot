#include "includes.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <queue>
#include <sched.h>
#include <sstream>
#include <thread>
#include <vector>

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "movetimings.hpp"
#include "search.hpp"

#define OUT_FILE "data.bullet.txt"
#define FIXED_NODES 10000
const int TT_SIZE = DEFAULT_TT_SIZE;

BoardState bs[64]; // boardstates for each thread

// Thread-safe queue for storing generated game data
class SafeQueue {
private:
	std::queue<std::string> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;

public:
	void push(const std::string &data) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(data);
		cond_.notify_one();
	}

	bool pop(std::string &data) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (queue_.empty()) {
			return false;
		}
		data = queue_.front();
		queue_.pop();
		return true;
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

	size_t size() {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}

	void wait_and_pop(std::string &data) {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { return !queue_.empty(); });
		data = queue_.front();
		queue_.pop();
	}
};

// Global variables
SafeQueue gameDataQueue;
std::atomic<int> totalPositions(0);
std::atomic<int> totalGames(0);
std::atomic<bool> shouldStop(false);
std::atomic<bool> stopWriter(false);
std::mutex printMutex;

// Worker thread function to generate games
void generateGames(int worker_id) {
#ifndef WINDOWS
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(worker_id, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
#endif

	while (!shouldStop.load()) {
		Board board = Board(TT_SIZE);
		// Generate random moves to start the game
		for (int i = 0; i < 8; i++) {
			pzstd::vector<Move> moves;
			board.legal_moves(moves);
			if (moves.size() == 0) {
				continue; // Skip this game and start a new one
			}
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

			if (board.threefold() || board.halfmove >= 100) {
				// Threefold repetition or 50-move rule
				res = "0.5";
				break;
			}

			// Search for a move
			std::pair<Move, Value> result = search_nodes(board, FIXED_NODES, bs[worker_id]);

			if (result.first == NullMove) {
				bool in_check = false;
				if (board.side == WHITE) {
					in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second > 0;
				} else {
					in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first > 0;
				}
				
				if (in_check) {
					// Checkmate
					res = (board.side == WHITE) ? "0.0" : "1.0";
				} else {
					// Stalemate
					res = "0.5";
				}
				break;
			}

			// Get the eval
			eval = result.second;
			if (board.side == BLACK)
				eval *= -1; // change eval to white perspective

			if (eval >= 10000) {
				res = "1.0";
				break;
			} else if (eval <= -10000) {
				res = "0.0";
				break;
			}

			bool in_check = false;
			if (board.side == WHITE) {
				in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(WHITE)])).second > 0;
			} else {
				in_check = board.control(__tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(BLACK)])).first > 0;
			}
			bool is_capture = (board.piece_boards[OPPOCC(board.side)] & square_bits(result.first.dst()));

			if (!in_check && !is_capture && result.first.type() == NORMAL) {
				// Store the position with its evaluation
				game.push_back({board.get_fen(), eval});
			}

			// Make the move
			board.make_move(result.first);
		}

		// Format the game data for writing
		std::stringstream gameData;
		for (auto &entry : game) {
			// [fen] | [eval] | [result]
			gameData << entry.first << " | " << entry.second / CP_SCALE_FACTOR << " | " << res << "\n";
		}

		// Push to the queue for writing
		if (game.size() > 0) {
			gameDataQueue.push(gameData.str());
			totalPositions += game.size();
			totalGames++;
		}
	}
	std::cout << "Worker " << worker_id << " finished." << std::endl;
}

// Writer thread function to handle file I/O
void writerThread(std::ofstream &outfile) {
	std::string data;
	std::string buffer;
	const size_t BUFFER_SIZE = 64 * 1024 * 1024; // 64MB buffer
	auto lastFlush = std::chrono::steady_clock::now();
	const auto FLUSH_INTERVAL = std::chrono::seconds(30); // Flush every 30 seconds
	
	while (!stopWriter.load() || !gameDataQueue.empty()) {
		if (gameDataQueue.pop(data)) {
			buffer += data;
			
			// Flush if buffer is large enough or enough time has passed
			auto now = std::chrono::steady_clock::now();
			if (buffer.size() >= BUFFER_SIZE || (now - lastFlush) >= FLUSH_INTERVAL) {
				outfile << buffer;
				outfile.flush();
				buffer.clear();
				lastFlush = now;
			}
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent CPU spinning
		}
	}
	
	// Final flush of remaining data
	if (!buffer.empty()) {
		outfile << buffer;
		outfile.flush();
	}
	
	std::cout << "Writer thread finished." << std::endl;
}

// Monitoring thread to print statistics
void monitorThread(std::chrono::steady_clock::time_point start) {
	auto prev = std::chrono::steady_clock::now();
	while (!stopWriter.load()) {
		// Use wall clock time instead of CPU time
		auto now = std::chrono::steady_clock::now();

		// Update stats every 5 minutes
		if (now - prev < std::chrono::seconds(300)) {
			continue; // Skip if not enough time has passed
		}
		prev = now;

		int positions = totalPositions.load();
		int games = totalGames.load();

		double elapsedSecs = std::chrono::duration<double>(now - start).count();

		std::lock_guard<std::mutex> lock(printMutex);
		std::cout << "Generated " << positions << " positions in " << games << " games in " << elapsedSecs << "s" << std::endl;
		std::cout << "Positions / second: ~" << (positions / elapsedSecs) << std::endl;
		std::cout << "Queue size: " << gameDataQueue.size() << std::endl << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

// Signal handler function
void signalHandler(int signal) {
	std::cout << "Stopping threads, please wait..." << std::endl;
	shouldStop.store(true);
}

int main(int argc, char *argv[]) {
	init_network(bs); // Initialize the neural network
	// Data generation script
	std::ofstream outfile(OUT_FILE, std::ios::app); // Append mode in case of restart

	const int NUM_THREADS = std::thread::hardware_concurrency();
	// const int NUM_THREADS = 8;

	std::cout << "PZChessBot " << VERSION << " parallelized data generation script" << std::endl;
	std::cout << "Using " << NUM_THREADS << " worker threads" << std::endl;
	std::cout << "I'm going to generate as much data as I can, until you stop me. Press Ctrl+C to stop." << std::endl << std::endl;

	// Set up random seed - different for each run
	srand(time(NULL));

	// Start timing using wall clock time
	auto start = std::chrono::steady_clock::now();

	// Set up signal handler for clean shutdown
	std::signal(SIGINT, signalHandler);

	// Create writer thread
	std::thread writer(writerThread, std::ref(outfile));

	// Create monitor thread
	std::thread monitor(monitorThread, start);

	// Create worker threads
	std::vector<std::thread> workers;
	for (int i = 0; i < NUM_THREADS; i++) {
		workers.push_back(std::thread(generateGames, i));
	}

	// Join threads when done (this won't happen unless SIGINT is received)
	for (auto &worker : workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}

	// Stop the writer thread
	stopWriter.store(true);

	if (monitor.joinable()) {
		monitor.join();
	}

	if (writer.joinable()) {
		writer.join();
	}

	// Final stats using wall clock time
	auto end = std::chrono::steady_clock::now();
	double elapsedSecs = std::chrono::duration<double>(end - start).count();

	int positions = totalPositions.load();
	int games = totalGames.load();

	std::cout << "Final stats:" << std::endl;
	std::cout << "Generated " << positions << " positions in " << games << " games in " << elapsedSecs << "s" << std::endl;
	std::cout << "Positions / second: " << (positions / elapsedSecs) << std::endl;

	outfile.close();
	return 0;
}
