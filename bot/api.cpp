#include "api.hpp"

#define REQUEST_URL (std::string) "https://lichess.org"

int __send_request(std::string url, std::string method) { /// TODO: json
	if (method == "GET") {
		cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Bearer{TOKEN});
		/// TODO: set response (r.text to json)
		return r.status_code;
	} else {
		cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Bearer{TOKEN});
		return r.status_code;
	}
}

int __send_request(std::string url, std::string method, cpr::Response &r) {
	if (method == "GET") {
		r = cpr::Get(cpr::Url{url}, cpr::Bearer{TOKEN});
		return r.status_code;
	} else {
		r = cpr::Post(cpr::Url{url}, cpr::Bearer{TOKEN});
		return r.status_code;
	}
}

int API::move(std::string game_id, std::string move, bool draw) {
	std::string url = REQUEST_URL + "/api/bot/game/" + game_id + "/move/" + move;
	if (draw)
		url += "?offeringDraw=true";
	return __send_request(url, "POST");
}

int API::chat(std::string game_id, int room, std::string message) {
	std::string url = REQUEST_URL + "/api/bot/game/" + game_id + "/chat";
	if (room == 0) {
		url += "/player";
	} else {
		url += "/spectator";
	}
	url += "?text=" + message;
	return __send_request(url, "POST");
}

int API::resign(std::string game_id) {
	std::string url = REQUEST_URL + "/api/bot/game/" + game_id + "/resign";
	return __send_request(url, "POST");
}

json API::get_challenges() {
	std::string url = REQUEST_URL + "/api/challenge";
	cpr::Response res;
	if (__send_request(url, "GET", res) == 200) {
		return json::parse(res.text);
	} else {
		return 0;
	}
}

int API::send_challenge(std::string username, bool rated, int time, int increment, std::string color, std::string variant) {
	std::string url = REQUEST_URL + "/api/bot/challenge/" + username;
	url += "?rated=" + std::to_string(rated);
	url += "&timeControl=" + std::to_string(time) + "+" + std::to_string(increment);
	url += "&color=" + color;
	url += "&variant=" + variant;
	return __send_request(url, "POST");
}

int API::accept_challenge(std::string challenge_id) {
	std::string url = REQUEST_URL + "/api/bot/challenge/" + challenge_id + "/accept";
	return __send_request(url, "POST");
}

API::Events::Events() {
	// make a request to the stream endpoint
	request = cpr::GetAsync(cpr::Url{REQUEST_URL + "/api/stream/event"}, cpr::Bearer{TOKEN}, cpr::WriteCallback{[this](std::string response, intptr_t userdata){return this->callback(response);}});
}

API::Events::~Events() {
	running = false;
	request.wait();
}

bool API::Events::callback(std::string header) {
	// read and parse until end of valid json and store the rest in residual
	json j;
	std::string s = residual + header;
	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '\n') {
			try {
				j = json::parse(s.substr(0, i));
				residual = s.substr(i + 1);
				s = residual;
				events.push_back(j);
			} catch (json::parse_error &e) {
				// TODO: handle error
			}
		}
	}
	return running;
}

std::vector<json> API::Events::get_events() {
	std::vector<json> copy{events};
	events.clear();
	return copy;
}

API::Game::Game(std::string game_id) {
	// make a request to the stream endpoint
	request = cpr::GetAsync(cpr::Url{REQUEST_URL + "/api/board/game/stream/" + game_id}, cpr::Bearer{TOKEN}, cpr::WriteCallback{[this](std::string response, intptr_t userdata){return this->callback(response);}});
}

API::Game::~Game() {
	running = false;
	request.wait();
}

bool API::Game::callback(std::string header) {
	// read and parse until end of valid json and store the rest in residual
	json j;
	std::string s = residual + header;
	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '\n') {
			try {
				j = json::parse(s.substr(0, i));
				residual = s.substr(i + 1);
				s = residual;
				events.push_back(j);
			} catch (json::parse_error &e) {
				// TODO: handle error
			}
		}
	}
	return running;
}

std::vector<json> API::Game::get_events() {
	std::vector<json> copy{events};
	events.clear();
	return copy;
}
