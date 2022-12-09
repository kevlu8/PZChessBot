#include "../engine/ab_engine/moves.hpp"
#include "../engine/ab_engine/search.hpp"
#include "../engine/fen.hpp"
#include "api.hpp"
#include <pthread.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// create a list of threads
// each thread will be a game
std::unordered_map<std::string, pthread_t> games;

// loop that handles game events and plays the game
void play(std::string game_id, bool color) {
	std::cout << "playing as " << (color ? "white" : "black") << std::endl;
	std::string moves = "";
	int nummoves = 0;
	char board[64];
	char metadata[3];
	char extra[2];
	memset(extra, 0, 69);
	// connect to the game stream
	API::Game game(game_id);
	// main loop
	std::vector<json> events;
	while (true) {
		game.get_events(events);
		for (json event : events) {
			std::cout << "play: " << event << std::endl;
			// if the event type is gameFull
			if (event["type"] == "gameFull") {
				// load the fen into the board
				if (event["initialFen"] != "startpos") {
					parse_fen(event["initialFen"], board, metadata, extra);
				} else {
					parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", board, metadata, extra);
				}
				event = event["state"];
				std::string history = event["moves"];
				moves = history;
				while (history.size()) {
					std::string move = history.substr(0, history.find(' '));
					history = history.substr(history.find(' ') + 1);
					make_move(move, "", board, metadata);
					nummoves++;
				}
			}
			if (event["type"] == "gameState") {
				if (nummoves % 2 != color) {
					std::cout << "our turn" << std::endl;
					std::string prev;
					if (moves.size())
						prev = moves.substr(moves.find_last_of(' ') + 1);
					else
						prev = "0000";
					std::string move = ab_search(board, 4, metadata, prev, color)[0].first;
					make_move(move, prev, board, metadata);
					API::move(game_id, move);
					if (moves.size())
						moves += " " + move;
					else
						moves = move;
					nummoves++;
				} else {
					std::cout << "their turn" << std::endl;
					std::string history = to_string(event["moves"]).substr(1, to_string(event["moves"]).size() - 2);
					if (history.size() != moves.size()) {
						std::string move, prev;
						if (moves.size()) {
							if (moves.size() > 4) {
								move = history.substr(moves.size() + 1);
								prev = moves[moves.size() - 5] == ' ' ? moves.substr(moves.size() - 4) : moves.substr(moves.size() - 5);
							} else {
								move = history.substr(moves.size() + 1);
								prev = moves;
							}
						} else {
							move = history;
							prev = "0000";
						}
						moves = history;
						nummoves++;
						make_move(move, prev, board, metadata);
						std::cout << "our turn" << std::endl;
						prev = move;
						move = ab_search(board, 4, metadata, prev, color)[0].first;
						std::cout << "best move: " << move << std::endl;
						make_move(move, prev, board, metadata);
						// for (int i = 0; i < 64; i++) {
						// 	std::cout << std::setw(3) << (int)board[i];
						// 	if (i % 8 == 7) {
						// 		std::cout << '\n';
						// 	}
						// }
						// std::cout << '\n';
						API::move(game_id, move);
						nummoves++;
					}
				}
			}
		}
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
}

void *play_helper(void *args) {
	std::cout << "helper" << std::endl;
	char *game_id = (char *)args;
	play(game_id, ((char *)args)[strlen(game_id) + 1]);
	free(args);
	return NULL;
}

// general event handler to dispatch jobs
void handle_event(json event) {
	if (event["type"] == "challenge") {
		API::accept_challenge(event["challenge"]["id"]);
	} else if (event["type"] == "gameStart") {
		std::cout << "game start" << std::endl;
		int len = to_string(event["game"]["gameId"]).size() - 2;
		char *args = (char *)malloc(len + 2);
		memcpy(args, to_string(event["game"]["gameId"]).substr(1, len).c_str(), len);
		args[len] = 0;
		args[len + 1] = event["game"]["color"] == "white";
		pthread_t t = pthread_create(&t, NULL, play_helper, args);
		games[event["game"]["gameId"]] = t;
	} else {
		std::cout << event << std::endl;
	}
}

int main() {
	// connect to the event stream
	API::Events listener;
	// get challenges
	json challenges = API::get_challenges()["in"];
	// for each challenge make a thread to handle it
	for (auto challenge : challenges) {
		// accept the challenge
		API::accept_challenge(challenge["id"]);
	}
	// main loop
	while (true) {
		// poll for incoming events
		std::vector<json> events;
		listener.get_events(events);
		// handle them
		for (auto event : events) {
			std::cout << event << std::endl;
			handle_event(event);
		}
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
	return 0;
}
