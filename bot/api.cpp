#include "api.hpp"

const std::string request_url = "https://lichess.org";

int __send_request(std::string url, std::string method) { /// TODO: json
	if (method == "GET") {
		cpr::Response r = cpr::Get(cpr::Url{url},
						  cpr::Bearer{TOKEN}
		);
		/// TODO: set response (r.text to json)
		return r.status_code;
	} else {
		cpr::Response r = cpr::Post(cpr::Url{url},
						  cpr::Bearer{TOKEN}
		);
		return r.status_code;
	}
}

int API::move(std::string game_id, std::string move, bool draw) {
	std::string url = request_url + "/api/bot/game/" + game_id + "/move/" + move;
	if (draw) url += "?offeringDraw=true";
	return __send_request(url, "POST");
}

int API::chat(std::string game_id, int room, std::string message) {
	std::string url = request_url + "/api/bot/game/" + game_id + "/chat";
	if (room == 0) {
		url += "/player";
	} else {
		url += "/spectator";
	}
	url += "?text=" + message;
	return __send_request(url, "POST");
}

int API::resign(std::string game_id) {
	std::string url = request_url + "/api/bot/game/" + game_id + "/resign";
	return __send_request(url, "POST");
}

// json API::get_challenges() {
// 	std::string url = request_url + "/api/bot/challenge";
//	json res;
// 	if (__send_request(url, "GET", &res) == 200)) {
// 		return res;
//	} else {
// 		return 0;
//	}
// }

int API::send_challenge(std::string username, bool rated, int time, int increment, 
	std::string color, std::string variant) {
	std::string url = request_url + "/api/bot/challenge/" + username;
	url += "?rated=" + std::to_string(rated);
	url += "&timeControl=" + std::to_string(time) + "+" + std::to_string(increment);
	url += "&color=" + color;
	url += "&variant=" + variant;
	return __send_request(url, "POST");
}

int API::accept_challenge(std::string challenge_id) {
	std::string url = request_url + "/api/bot/challenge/" + challenge_id + "/accept";
	return __send_request(url, "POST");
}