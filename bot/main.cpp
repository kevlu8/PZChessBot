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
	std::vector<std::string> moves;
	int nummoves = 0;
	char original_board[64];
	char original_metadata[3];
	char original_extra[2];
	parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", original_board, original_metadata, original_extra);
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
			if (event["type"] == "chatLine" || event["type"] == "opponentGone")
				continue;
			if (event["type"] == "gameFinish") {
				return;
			}
			if (event["type"] == "gameFull") {
				event = event["state"];
			}
			memcpy(board, original_board, 64);
			memcpy(metadata, original_metadata, 3);
			memcpy(extra, original_extra, 2);
			// split the moves into an array of moves and in the process update the internal board
			std::string moves_str = event["moves"];
			std::string move, prev = "0000";
			for (int i = 0; i < moves_str.size(); i++) {
				if (moves_str[i] == ' ') {
					moves.push_back(move);
					make_move(move, prev, board, metadata);
					prev = move;
					move = "";
				} else {
					move += moves_str[i];
				}
			}
			if (move != "") {
				moves.push_back(move);
				make_move(move, prev, board, metadata);
				prev = move;
			}
			// if its our turn
			if (moves.size() % 2 != color) {
				move = ab_search(board, 4, metadata, prev, color);
				if (move == "resign")
					API::resign(game_id);
				else
					API::move(game_id, move);
			}
			moves.clear();
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
