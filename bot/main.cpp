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
	// connect to the game stream
	API::Game game(game_id);

}

// general event handler to dispatch jobs
void handle_event(json event) {
	if (event["type"] == "gameStart") {
		void *args = malloc(9);
		args[0] = &event["game"]["gameId"];
		args[]
		pthread_t t = pthread_create(&t, NULL, play_helper, (void *)&event["game"]["gameId"], event["game"]["color"] == "white");
		games[event["game"]["gameId"]] = t;
	}
}

int main() {
	// connect to the event stream
	API::Events listener;
	// main loop
	while (true) {
		// get challenges
		json challenges = API::get_challenges();
		// for each challenge make a thread to handle it
		for (auto challenge : challenges) {
			// accept the challenge
			API::accept_challenge(challenge["id"]);
		}
		// poll for incoming events
		std::vector<json> events = listener.get_events();
		// handle them
		for (auto event : events) {
			handle_event(event);
		}
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
	return 0;
}
