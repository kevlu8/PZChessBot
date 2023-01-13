#pragma once

#include "../token"
#include "json.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <string>
using json = nlohmann::json;

int __send_request(std::string, std::string);

namespace API {
	/**
	 * @brief Send a move to the server
	 *
	 * @param game_id The ID of the game
	 * @param move The move to send in UCI format (e.g. e2e4)
	 * @param draw Whether the player is offering a draw
	 * @return int The status code of the request
	 */
	int move(std::string, std::string, bool = false);

	/**
	 * @brief Send a chat message to the server
	 *
	 * @param game_id The ID of the game
	 * @param room The room to send the message to (0 for player, 1 for spectator)
	 * @param message The message to send
	 * @return int The status code of the request
	 */
	int chat(std::string, int, std::string);

	/**
	 * @brief Send a resign request to the server
	 * @param game_id The ID of the game
	 * @return int The status code of the request
	 */
	int resign(std::string);

	/**
	 * @brief Get challenges from the server
	 * @return json JSON object containing the challenges
	 */
	json get_challenges();

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
	int send_challenge(std::string, bool, int, int, std::string = "random", std::string = "standard");

	/**
	 * @brief Accept a challenge from the server
	 * @param challenge_id The ID of the challenge
	 * @return int The status code of the request
	 */
	int accept_challenge(std::string);

	/**
	 * @brief Decline a challenge from the server
	 * @param challenge_id The ID of the challenge
	 * @param reason The reason for declining the challenge
	 * @return int The status code of the request
	 */
	int decline_challenge(std::string, std::string);

	class Events {
	private:
		bool running = true;
		bool callback(std::string);
		std::string residual;
		std::vector<json> events;
		cpr::AsyncResponse request;

	public:
		/**
		 * @brief Construct a new Events object
		 */
		Events();

		/**
		 * @brief Destroy the Events object
		 */
		~Events();

		/**
		 * @brief Get the events from the server
		 *
		 * @param out A vector of JSON objects containing the events
		 */
		void get_events(std::vector<json> &);
	};

	class Game {
	private:
		bool running = true;
		bool callback(std::string);
		std::string residual;
		std::vector<json> events;
		cpr::AsyncResponse request;

	public:
		/**
		 * @brief Construct a new Game object
		 * @param game_id The ID of the game
		 */
		Game(std::string);

		/**
		 * @brief Destroy the Game object
		 */
		~Game();

		/**
		 * @brief Get the events from the server
		 *
		 * @param out A vector of JSON objects containing the events
		 */
		void get_events(std::vector<json> &);
	};
}
