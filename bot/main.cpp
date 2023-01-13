#include "../engine/bitboard.hpp"
#include "../engine/search.hpp"
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
	Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	std::vector<std::string> moves;
	moves.reserve(512);
	std::string moves_str, move;
	// connect to the game stream
	API::Game game(game_id);
	// main loop
	std::vector<json> events;
	while (true) {
		game.get_events(events);
		for (json event : events) {
			std::cout << "play: " << event << '\n';
			if (event["type"] == "chatLine" || event["type"] == "opponentGone")
				continue;
			if (event["type"] == "gameFinish")
				return;
			if (event["type"] == "gameFull")
				event = event["state"];
			// split the moves into an array of moves and in the process update the internal board
			moves_str = event["moves"];
			for (int i = 0; i < moves_str.size(); i++) {
				if (moves_str[i] == ' ') {
					moves.push_back(move);
					board.make_move(parse_move(board, move));
					move.clear();
				} else {
					move += moves_str[i];
				}
			}
			if (move != "") {
				moves.push_back(move);
				board.make_move(parse_move(board, move));
				move.clear();
			}
			// if its our turn
			if (moves.size() % 2 != color) {
				move = stringify_move(ab_search(board, 7).second);
				if (move == "----")
					API::resign(game_id);
				else
					API::move(game_id, move);
			}
			moves.clear();
			move.clear();
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
		if (event["challenge"]["variant"]["short"] == "Std")
			API::accept_challenge(event["challenge"]["id"]);
		else
			API::decline_challenge(event["challenge"]["id"], "variant");
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
		if (challenge["variant"]["short"] == "Std")
			API::accept_challenge(challenge["id"]);
		else
			API::decline_challenge(challenge["id"], "variant");
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
