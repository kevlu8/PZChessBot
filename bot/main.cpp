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
void play(std::string game_id, bool color, uint8_t depth, json *initialEvent) {
	std::cout << "playing as " << (color ? "white" : "black") << std::endl;
	Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	std::vector<std::string> moves, prev_moves;
	std::string moves_str, move;
	// connect to the game stream
	API::Game game(game_id);
	std::cout << "connected to game " << game_id << std::endl;
	// main loop
	ListNode *head = new ListNode{initialEvent, nullptr, nullptr};
	ListNode *tail = head;
	while (true) {
		moves_str = "";
		game.get_events(&head, &tail);
		while (head != nullptr) {
			json event = *head->val;
			delete head->val;
			if (head->next != nullptr) {
				head = head->next;
				delete head->prev;
				head->prev = nullptr;
			} else {
				delete head;
				head = nullptr;
			}
			std::cout << "play: " << event << '\n';
			if (event["type"] == "chatLine" || event["type"] == "opponentGone")
				continue;
			else if (event["type"] == "gameFinish")
				break;
			else if (event["type"] == "gameFull") {
				board.load_fen(event["initialFen"]);
				event = event["state"];
			} else if (event["type"] == "gameStart") {
				if (event["game"]["hasMoved"] != "")
					continue;
				std::cout << "game start packet" << std::endl;
				if (!event["game"]["isMyTurn"])
					continue;
				moves_str = "null";
			}
			// split the moves into an array of moves
			if (moves_str == "") {
				moves_str = event["moves"];
				for (int i = 0; i < moves_str.size(); i++) {
					if (moves_str[i] == ' ') {
						moves.push_back(move);
						move.clear();
					} else {
						move += moves_str[i];
					}
				}
				if (move != "") {
					moves.push_back(move);
					move.clear();
				}
				if (moves.size() != prev_moves.size()) {
					for (int i = prev_moves.size(); i < moves.size(); i++)
						board.make_move(parse_move(board, moves[i]));
					prev_moves = moves;
				}
			}
			// if its our turn
			std::cout << "color: " << color << " moves.size(): " << moves.size() << std::endl;
			if (moves.size() % 2 != color) {
				std::cout << "thinking" << std::endl;
				move = stringify_move(ab_search(board, depth).second);
				board.print_board();
				if (move != "----" && move != "0000")
					API::move(game_id, move);
				else
					break;
			}
			moves.clear();
			move.clear();
		}
		tail = head;
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
	delete initialEvent;
}

void *play_helper(void *args) {
	char *game_id = (char *)args;
	play(game_id, ((char *)args)[strlen(game_id) + 2], ((char *)args)[strlen(game_id) + 1], *(json **)(args + strlen(game_id) + 3));
	free(args);
	return NULL;
}

// general event handler to dispatch jobs
void handle_event(json event) {
	if (event["type"] == "challenge") {
		if (event["challenge"]["challenger"]["id"] != "wdotmathree")
			API::decline_challenge(event["challenge"]["id"], "generic");
		else
			API::accept_challenge(event["challenge"]["id"]);
		// if (event["challenge"]["variant"]["short"] == "Std" || event["challenge"]["challenger"]["id"] == "wdotmathree")
		// 	API::accept_challenge(event["challenge"]["id"]);
		// else
		// 	API::decline_challenge(event["challenge"]["id"], "standard");
	} else if (event["type"] == "gameStart") {
		std::cout << "game start" << std::endl;
		int len = to_string(event["game"]["gameId"]).size() - 2;
		char *args = (char *)malloc(len + 2 + sizeof(void *));
		memcpy(args, to_string(event["game"]["gameId"]).substr(1, len).c_str(), len);
		args[len] = 0;
		if (event["game"]["speed"] == "bullet")
			args[len + 1] = 9;
		else if (event["game"]["speed"] == "blitz")
			args[len + 1] = 6;
		else if (event["game"]["speed"] == "rapid")
			args[len + 1] = 11;
		else
			args[len + 1] = 12;
		args[len + 2] = event["game"]["color"] == "white";
		*(json **)(args + len + 3) = new json(event);
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
		ListNode *head = nullptr;
		ListNode *tail = nullptr;
		listener.get_events(&head, &tail);
		// handle them
		while (head != nullptr) {
			json event = *head->val;
			delete head->val;
			if (head->next != nullptr) {
				head = head->next;
				delete head->prev;
				head->prev = nullptr;
			} else {
				delete head;
				head = nullptr;
			}
			std::cout << event << std::endl;
			handle_event(event);
		}
		tail = head;
		// sleep for 1ms to prevent cpu usage
		usleep(1000);
	}
	return 0;
}
