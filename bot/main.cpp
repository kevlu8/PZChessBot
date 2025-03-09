#include "../engine/bitboard.hpp"
#include "../engine/movetimings.hpp"
#include "../engine/search.hpp"
#include "api.hpp"
#include "book.hpp"
#include <sstream>
#include <unordered_map>
#include <vector>

// create a list of threads
// each thread will be a game
std::unordered_map<std::string, std::unique_ptr<std::thread>> games;

void play(const json &initialEvent) {
	std::string game_id = initialEvent["game"]["gameId"];
	bool color = initialEvent["game"]["color"] == "white";
	std::cout << "playing as " << (color ? "white" : "black") << std::endl;

	Board board;
	std::vector<std::string> moves;
	// Connect to the game stream
	JSONStream game(API_REQUEST_URL "/api/stream/game/" + game_id);
	std::cout << "connected to game " << game_id << std::endl;

	while (json event = game.waitMsg()) {
		if (event["type"] == "chatLine" || event["type"] == "opponentGone") {
			continue;
		} else if (event["type"] == "gameFinish") {
			break;
		} else if (event["type"] == "gameState") {
			event = event["state"];
		} else if (event["type"] == "gameFull") {
			event = event["state"];
		} else if (event["type"] == "gameStart") {
			if (event["game"]["hasMoved"] != "")
				continue;
			if (!event["game"]["isMyTurn"])
				continue;
		}
		if (event.contains("error")) {
			API::resign(game_id);
			break;
		}

		// Update our board
		std::stringstream ss((std::string)event["moves"]);
		for (int i = 0; !ss.eof(); i++) {
			std::string move;
			ss >> move;
			if (i >= moves.size()) {
				moves.push_back(move);
				board.make_move(Move::from_string(move, &board));
			}
		}

		// If its our turn
		if (moves.size() % 2 != color) {
			std::pair<Move, bool> book = {NullMove, false};
			if (moves.size() <= 30 && (book = book_move(moves, board)).second) {
				std::string move = book.first.to_string();
				std::cout << "book move: " << move << std::endl;
				API::move(game_id, move);
			} else {
				auto tmp = search(board, timetonodes(color ? event["wtime"] : event["btime"], 1));
				std::string move = tmp.first.to_string();
				std::cout << "move: " << move << "eval: " << tmp.second << std::endl;
				if (move != "----" && move != "0000")
					API::move(game_id, move);
				else
					break;
			}
		}
	}

	// Remove the game from the list
	games.erase(game_id);
}

int main() {
	clock_t start = clock();
	init_book();
	std::cout << "book loaded in " << (double)(clock() - start) * 1000 / CLOCKS_PER_SEC << "ms" << std::endl;

	// Deal with existing challenges
	json challenges = API::get_challenges()["in"];
	for (auto challenge : challenges) {
		if (challenge["challenger"]["id"] != "kevlu8" && challenge["challenger"]["id"] != "wdotmathree")
			API::decline_challenge(challenge["id"], "generic");
		else
			API::accept_challenge(challenge["id"]);
		// if (challenge["variant"]["short"] == "Std")
		// 	API::accept_challenge(challenge["id"]);
		// else
		// 	API::decline_challenge(challenge["id"], "variant");
	}

	// Process events
	while (true) {
		try {
			JSONStream listener(API_REQUEST_URL "/api/stream/event");
			while (json event = listener.waitMsg()) {
				if (event["type"] == "challenge") {
					if (event["challenge"]["challenger"]["id"] != "kevlu8" && event["challenge"]["challenger"]["id"] != "wdotmathree")
						API::decline_challenge(event["challenge"]["id"], "generic");
					else
						API::accept_challenge(event["challenge"]["id"]);
					// if (event["challenge"]["variant"]["short"] == "Std")
					// 	API::accept_challenge(event["challenge"]["id"]);
					// else
					// 	API::decline_challenge(event["challenge"]["id"], "variant");
				} else if (event["type"] == "gameStart") {
					std::cout << "game start" << std::endl;
					games[event["game"]["gameId"]] = std::make_unique<std::thread>(play, event);
				} else {
					// std::cout << event << std::endl;
				}
			}
		} catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	}
	return 0;
}
