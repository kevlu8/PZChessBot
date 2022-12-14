#include "api.hpp"

#define REQUEST_URL (std::string) "https://lichess.org"

int __send_request(std::string url, std::string method) {
	if (method == "GET") {
		cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Bearer{TOKEN});
		if (r.status_code != 200) {
			std::cerr << method << ' ' << url << ' ' << r.text << std::endl;
		}
		return r.status_code;
	} else {
		cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Bearer{TOKEN});
		if (r.status_code != 200) {
			std::cerr << method << ' ' << url << ' ' << r.text << std::endl;
		}
		return r.status_code;
	}
}

int __send_request(std::string url, std::string method, cpr::Response &r) {
	if (method == "GET") {
		r = cpr::Get(cpr::Url{url}, cpr::Bearer{TOKEN});
		if (r.status_code != 200) {
			std::cerr << method << ' ' << url << ' ' << r.text << std::endl;
		}
		return r.status_code;
	} else {
		r = cpr::Post(cpr::Url{url}, cpr::Bearer{TOKEN});
		if (r.status_code != 200) {
			std::cerr << method << ' ' << url << ' ' << r.text << std::endl;
		}
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
	std::string url = REQUEST_URL + "/api/challenge/" + challenge_id + "/accept";
	return __send_request(url, "POST");
}

int API::decline_challenge(std::string challenge_id) {
	std::string url = REQUEST_URL + "/api/challenge/" + challenge_id + "/decline";
	cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Bearer{TOKEN}, cpr::Body{"reason: \"variant\""}); /// TODO: fix this thing
	return r.status_code;
}

API::Events::Events() {
	// make a request to the stream endpoint
	request = cpr::GetAsync(cpr::Url{REQUEST_URL + "/api/stream/event"}, cpr::Bearer{TOKEN}, cpr::WriteCallback{[this](std::string response, intptr_t userdata) { return this->callback(response); }});
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
				if (s.substr(0, i) != "\n") {
					j = json::parse(s.substr(0, i));
					events.push_back(j);
				}
				residual = s.substr(i + 1);
				s = residual;
			} catch (json::parse_error &e) {
				// TODO: handle error
			}
		}
	}
	return running;
}

void API::Events::get_events(std::vector<json> &out) {
	out = events;
	events.clear();
}

API::Game::Game(std::string game_id) {
	std::cout << "gameid: " << game_id << std::endl;
	// make a request to the stream endpoint
	request = cpr::GetAsync(cpr::Url{REQUEST_URL + "/api/bot/game/stream/" + game_id}, cpr::Bearer{TOKEN}, cpr::WriteCallback{[this](std::string response, intptr_t userdata) { return this->callback(response); }});
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
				if (s.substr(0, i) != "\n") {
					j = json::parse(s.substr(0, i));
					events.push_back(j);
				}
				residual = s.substr(i + 1);
				s = residual;
			} catch (json::parse_error &e) {
				// TODO: handle error
			}
		}
	}
	return running;
}

void API::Game::get_events(std::vector<json> &out) {
	out = events;
	events.clear();
}
