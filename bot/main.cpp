#include "api.hpp"
#include "../engine/fen.hpp"
#include "../engine/ab_engine/moves.hpp"
#include "../engine/ab_engine/search.hpp"
#include <pthread.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// create a list of threads
// each thread will be a game
std::unordered_map<std::string, pthread_t> games;

// loop that handles game events and plays the game
void play(std::string game_id, bool color) {
	std::cout << "playing" << std::endl;
	std::string moves = "";
	int nummoves = 0;
	char board[64];
	char metadata[3];
	char extra[2];
	memset(extra + 1, 0, 69);
	// connect to the game stream
	API::Game game(game_id);
	// main loop
	std::vector<json> events;
	while (true) {
		game.get_events(events);
		for (json event : events) {
			std::cout << event << std::endl;
			// if the event type is gameFull
			if (event["type"] == "gameFull") {
				// load the fen into the board
				if (event["initialFen"] != "startpos")
					parse_fen(event["initialFen"], board, metadata, extra);
				else
					parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1", board, metadata, extra);
				event = event["state"];
			}
			if (event["type"] == "gameState") {
				if (nummoves == 0 && color) {
					// get the move
					std::string move = ab_search(board, 4, metadata, "0000", true)[0].first;
					// send the move
					API::move(game_id, move);
					// add the move to the list of moves
					moves = move;
				}
				// if it's our turn
				else if (nummoves % 2 != color) {
					// get the move
					std::string move = ab_search(board, 4, metadata, moves[moves.size() - 5] == ' ' ? moves.substr(moves.size() - 4, 4) : moves.substr(moves.size() - 5, 5), color)[0].first;
					// send the move
					API::move(game_id, move);
					// add the move to the list of moves
					moves += " " + move;
				} else {
					moves = event["moves"];
				}
			}
		}
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
		// std::cout << event << std::endl; 
	}
}

int main() {
	// connect to the event stream
	API::Events listener;
	// main loop
	while (true) {
		// get challenges
		json challenges = API::get_challenges()["in"];
		// for each challenge make a thread to handle it
		for (auto challenge : challenges) {
			// accept the challenge
			API::accept_challenge(challenge["id"]);
		}
		// poll for incoming events
		std::vector<json> events;
		listener.get_events(events);
		// handle them
		for (auto event : events) {
			handle_event(event);
		}
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
	return 0;
}
