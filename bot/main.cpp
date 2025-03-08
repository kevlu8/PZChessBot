// #include "../engine/bitboard.hpp"
// #include "../engine/search.hpp"
// #include "../engine/movetimings.hpp"
// #include "api.hpp"
// #include "book.hpp"
// #include <pthread.h>
// #include <signal.h>
// #include <unistd.h>
// #include <unordered_map>
// #include <vector>

// // create a list of threads
// // each thread will be a game
// std::unordered_map<std::string, pthread_t> games;

// // loop that handles game events and plays the game
// void play(std::string game_id, bool color, uint8_t depth, json *initialEvent) {
// 	std::cout << "playing as " << (color ? "white" : "black") << std::endl;
// 	Board board;
// 	std::vector<std::string> moves, prev_moves;
// 	std::string moves_str, move;
// 	// connect to the game stream
// 	API::Game game(game_id);
// 	std::cout << "connected to game " << game_id << std::endl;
// 	// main loop
// 	ListNode *head = new ListNode{initialEvent, nullptr, nullptr};
// 	ListNode *tail = head;
// 	bool run = true;
// 	int timeleft = 0;
// 	while (run) {
// 		moves_str = "";
// 		game.get_events(&head, &tail);
// 		while (head != nullptr) {
// 			json event = *head->val;
// 			delete head->val;
// 			if (head->next != nullptr) {
// 				head = head->next;
// 				delete head->prev;
// 				head->prev = nullptr;
// 			} else {
// 				delete head;
// 				head = nullptr;
// 			}
// 			// std::cout << "play: " << event << '\n';
// 			if (event["type"] == "chatLine" || event["type"] == "opponentGone")
// 				continue;
// 			else if (event["type"] == "gameFinish") {
// 				run = false;
// 				break;
// 			} else if (event["type"] == "gameState") {
// 				// if (event.contains("status") && (event["status"] == "resign" || event["status"] == "mate" || event["status"] == "draw" || event["status"] ==
// "stalemate")) {
// 				// 	run = false;
// 				// 	break;
// 				// }
// 				if (color) timeleft = event["wtime"];
// 				else timeleft = event["btime"];
// 			} else if (event["type"] == "gameFull") {
// 				event = event["state"];
// 			} else if (event["type"] == "gameStart") {
// 				// board.load_fen(event["initialFen"]);
// 				prev_moves.clear();

// 				if (event["game"]["hasMoved"] != "")
// 					continue;
// 				std::cout << "game start packet" << std::endl;
// 				if (!event["game"]["isMyTurn"])
// 					continue;
// 			}
// 			if (event.contains("error")) {
// 				API::resign(game_id);
// 				run = false;
// 				break;
// 			}
// 			// split the moves into an array of moves
// 			moves_str = event["moves"];
// 			for (int i = 0; i < moves_str.size(); i++) {
// 				if (moves_str[i] == ' ') {
// 					moves.push_back(move);
// 					move.clear();
// 				} else {
// 					move += moves_str[i];
// 				}
// 			}
// 			if (move != "") {
// 				moves.push_back(move);
// 				move.clear();
// 			}
// 			if (moves.size() != prev_moves.size()) {
// 				for (int i = prev_moves.size(); i < moves.size(); i++) {
// 					// std::cout << "making move: " << moves[i] << std::endl;
// 					board.make_move(Move::from_string(moves[i], &board));
// 					board.commit();
// 					prev_moves.push_back(moves[i]);
// 				}
// 			}
// 			// if its our turn
// 			if (moves.size() % 2 != color) {
// 				std::pair<Move, bool> book = {NullMove, false};
// 				if (moves.size() <= 30 && (book = book_move(moves, board)).second) {
// 					move = book.first.to_string();
// 					std::cout << "book move: " << move << std::endl;
// 					API::move(game_id, move);
// 				} else {
// 					auto tmp = search(board, timetonodes(timeleft, 1));
// 					move = tmp.first.to_string();
// 					std::cout << "move: " << move << "eval: " << tmp.second << std::endl;
// 					if (move != "----" && move != "0000")
// 						API::move(game_id, move);
// 					else
// 						break;
// 				}
// 			}
// 			moves.clear();
// 			move.clear();
// 		}
// 		tail = head;
// 		// sleep for 1ms to prevent cpu usage
// 		usleep(1000);
// 	}
// 	delete initialEvent;
// }

// void *play_helper(void *args) {
// 	char *game_id = (char *)args;
// 	play(game_id, ((char *)args)[strlen(game_id) + 2], ((char *)args)[strlen(game_id) + 1], *(json **)(args + strlen(game_id) + 3));
// 	free(args);
// 	return NULL;
// }

// // general event handler to dispatch jobs
// void handle_event(json event) {
// 	if (event["type"] == "challenge") {
// 		if (event["challenge"]["challenger"]["id"] != "kevlu8" && event["challenge"]["challenger"]["id"] != "wdotmathree")
// 			API::decline_challenge(event["challenge"]["id"], "generic");
// 		else
// 			API::accept_challenge(event["challenge"]["id"]);
// 		// if (event["challenge"]["variant"]["short"] == "Std")
// 		// 	API::accept_challenge(event["challenge"]["id"]);
// 		// else
// 		// 	API::decline_challenge(event["challenge"]["id"], "variant");
// 	} else if (event["type"] == "gameStart") {
// 		std::cout << "game start" << std::endl;
// 		int len = to_string(event["game"]["gameId"]).size() - 2;
// 		char *args = (char *)malloc(len + 2 + sizeof(void *));
// 		memcpy(args, to_string(event["game"]["gameId"]).substr(1, len).c_str(), len);
// 		args[len] = 0;
// 		if (event["game"]["speed"] == "bullet")
// 			args[len + 1] = 4;
// 		else if (event["game"]["speed"] == "blitz")
// 			args[len + 1] = 6;
// 		else if (event["game"]["speed"] == "rapid")
// 			args[len + 1] = 11;
// 		else
// 			args[len + 1] = 12;
// 		args[len + 2] = event["game"]["color"] == "white";
// 		*(json **)(args + len + 3) = new json(event);
// 		pthread_t t = pthread_create(&t, NULL, play_helper, args);
// 		games[event["game"]["gameId"]] = t;
// 	} else {
// 		// std::cout << event << std::endl;
// 	}
// }

// int main() {
// 	clock_t start = clock();
// 	init_book();
// 	std::cout << "book loaded in " << (double)(clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
// 	// connect to the event stream
// 	API::Events listener;
// 	// get challenges
// 	json challenges = API::get_challenges()["in"];
// 	// for each challenge make a thread to handle it
// 	for (auto challenge : challenges) {
// 		if (challenge["challenger"]["id"] != "kevlu8" && challenge["challenger"]["id"] != "wdotmathree")
// 			API::decline_challenge(challenge["id"], "generic");
// 		else
// 			API::accept_challenge(challenge["id"]);
// 		// // accept the challenge
// 		// if (challenge["variant"]["short"] == "Std")
// 		// 	API::accept_challenge(challenge["id"]);
// 		// else
// 		// 	API::decline_challenge(challenge["id"], "variant");
// 	}
// 	// main loop
// 	while (true) {
// 		// poll for incoming events
// 		ListNode *head = nullptr;
// 		ListNode *tail = nullptr;
// 		listener.get_events(&head, &tail);
// 		// handle them
// 		while (head != nullptr) {
// 			json event = *head->val;
// 			delete head->val;
// 			if (head->next != nullptr) {
// 				head = head->next;
// 				delete head->prev;
// 				head->prev = nullptr;
// 			} else {
// 				delete head;
// 				head = nullptr;
// 			}
// 			// std::cout << event << std::endl;
// 			handle_event(event);
// 		}
// 		tail = head;
// 		// sleep for 1ms to prevent cpu usage
// 		usleep(1000);
// 	}
// 	return 0;
// }

#include "net.hpp"

int main() {
	JSONStream stream("https://lichess.org/api/stream/event");
	while (true) {
		json msg = stream.waitMsg();
		std::cout << msg << std::endl;
	}
	return 0;
}
