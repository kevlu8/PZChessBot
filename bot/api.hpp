#include <string>
#include <cpr/cpr.h>
// #include "json.hpp"

const std::string request_url = "https://lichess.org";
const std::string TOKEN = "";

int __send_request(std::string url, std::string method);

namespace API {
	/**
	 * @brief Send a move to the server
	 * 
	 * @param game_id The ID of the game
	 * @param move The move to send in UCI format (e.g. e2e4)
	 * @param draw Whether the player is offering a draw
	 * @return int The status code of the request
	 */
	int move(std::string game_id, std::string move, bool draw);

	/**
	 * @brief Send a chat message to the server
	 * 
	 * @param game_id The ID of the game
	 * @param room The room to send the message to (0 for player, 1 for spectator)
	 * @param message The message to send
	 * @return int The status code of the request
	 */
	int chat(std::string game_id, int room, std::string message);

	/**
	 * @brief Send a resign request to the server
	 * @param game_id The ID of the game
	 * @return int The status code of the request
	 */
	int resign(std::string game_id);

	// fuck we're gonna need json
	/**
	 * @brief Get challenges from the server
	 * @return json JSON object containing the challenges
	 */
	// json get_challenges();

	/**
	 * @brief Send a challenge to the server
	 * @param username The username of the player to challenge
	 * @param rated Whether the game should be rated
	 * @param time The initial time in seconds
	 * @param increment The increment in seconds
	 * @param color The color to play as
	 * @param variant The variant to play (so far, only standard is supported)
	 * @return int The status code of the request
	 */
	int send_challenge(std::string username, bool rated, int time, int increment, 
		std::string color="random", std::string variant="standard"
	);

	/**
	 * @brief Accept a challenge from the server
	 * @param challenge_id The ID of the challenge
	 * @return int The status code of the request
	 */
	int accept_challenge(std::string challenge_id);


}