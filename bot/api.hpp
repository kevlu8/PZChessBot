#pragma once

#include "../token"
#include "json.hpp"
#include <curl/curl.h>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
using json = nlohmann::json;

#define API_REQUEST_URL "https://lichess.org"

namespace API {

	/**
	 * @brief Send a move to the server
	 * @note This function blocks until the move is successfully sent (may never return)
	 *
	 * @param game_id The ID of the game
	 * @param move The move to send in UCI format (e.g. e2e4)
	 * @param draw Whether the player is offering a draw
	 * @return The status code of the request
	 */
	int move(std::string, std::string, bool = false);

	/**
	 * @brief Send a chat message to the server
	 * @note This function blocks until the move is successfully sent (may never return)
	 *
	 * @param game_id The ID of the game
	 * @param room The room to send the message to (0 for player, 1 for spectator)
	 * @param message The message to send
	 * @return The status code of the request
	 */
	int chat(std::string, int, std::string);

	/**
	 * @brief Send a resign request to the server
	 * @note This function blocks until the move is successfully sent (may never return)
	 *
	 * @param game_id The ID of the game
	 * @return The status code of the request
	 */
	int resign(std::string);

	/**
	 * @brief Get challenges from the server
	 *
	 * @return JSON object containing the challenges
	 */
	json get_challenges();

	/**
	 * @brief Send a challenge to the server
	 *
	 * @param username The username of the player to challenge
	 * @param rated Whether the game should be rated
	 * @param time The initial time in seconds
	 * @param increment The increment in seconds
	 * @param color The color to play as
	 * @param variant The variant to play (so far, only standard is supported)
	 * @return The status code of the request
	 */
	int send_challenge(std::string, bool, int, int, std::string = "random", std::string = "standard");

	/**
	 * @brief Accept a challenge from the server
	 *
	 * @param challenge_id The ID of the challenge
	 * @return The status code of the request
	 */
	int accept_challenge(std::string);

	/**
	 * @brief Decline a challenge from the server
	 *
	 * @param challenge_id The ID of the challenge
	 * @param reason The reason for declining the challenge
	 * @return The status code of the request
	 */
	int decline_challenge(std::string, std::string);

	/**
	 * @brief Query the opening book for a move
	 *
	 * @param moves The moves leading up to the current position
	 * @return JSON object containing the response
	 */
	json book(std::string);

	class StreamError : public std::exception {
	private:
		std::string message;

	public:
		StreamError(std::string msg) : message(msg) {};
		const char *what() const noexcept override {
			return message.c_str();
		};
	};
}

class JSONStream {
private:
	std::string url;
	CURL *curl;
	std::string buffer;
	std::deque<json> msgQueue;
	std::thread t;
	CURLcode res = CURLE_OK;

	int status = -1;

	static size_t write_callback(char *, size_t, size_t, JSONStream *);

public:
	/**
	 * @brief Construct a new JSONStream object
	 *
	 * @param url The url to stream from
	 */
	JSONStream(std::string);
	~JSONStream();

	/**
	 * @brief Blocks until a message is available or stream is closed
	 *
	 * @return The next message, or an empty json object if the stream is closed
	 */
	json waitMsg();
};
