#include "api.hpp"

int _send_get(std::string url, std::string *response) {
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT;

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Authorization: Bearer " TOKEN);
	headers = curl_slist_append(headers, "Accept: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(
		curl, CURLOPT_WRITEFUNCTION,
		+[](char *contents, size_t size, size_t nmemb, std::string *buf) {
			if (buf)
				buf->append(contents, size * nmemb);
			return size * nmemb;
		}
	);
	CURLcode e;
	while (true) {
		e = curl_easy_perform(curl);
		if (e == CURLE_OK)
			break;
		std::cerr << "Failed to GET " + url + ": " << curl_easy_strerror(e) << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	int status;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	curl_easy_cleanup(curl);
	return status;
}

int _send_post(std::string url, std::string body, std::string *response) {
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT;

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Authorization: Bearer " TOKEN);
	headers = curl_slist_append(headers, "Accept: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(
		curl, CURLOPT_WRITEFUNCTION,
		+[](char *contents, size_t size, size_t nmemb, std::string *buf) {
			if (buf)
				buf->append(contents, size * nmemb);
			return size * nmemb;
		}
	);
	CURLcode e;
	while (true) {
		e = curl_easy_perform(curl);
		if (e == CURLE_OK)
			break;
		std::cerr << "Failed to POST " + url + ": " << curl_easy_strerror(e) << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	int status;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	curl_easy_cleanup(curl);
	return status;
}

int API::move(std::string game_id, std::string move, bool draw) {
	std::string url = API_REQUEST_URL "/api/bot/game/" + game_id + "/move/" + move;
	if (draw)
		url += "?offeringDraw=true";
	return _send_post(url, "", nullptr);
}

int API::chat(std::string game_id, int room, std::string message) {
	std::string url = API_REQUEST_URL "/api/bot/game/" + game_id + "/chat";
	if (room == 0) {
		url += "/player";
	} else if (room == 1) {
		url += "/spectator";
	} else {
		throw std::invalid_argument("Invalid room id");
	}
	url += "?text=" + message;
	return _send_post(url, "", nullptr);
}

int API::resign(std::string game_id) {
	std::string url = API_REQUEST_URL "/api/bot/game/" + game_id + "/resign";
	return _send_post(url, "", nullptr);
}

json API::get_challenges() {
	std::string url = API_REQUEST_URL "/api/challenge";
	std::string res;
	_send_get(url, &res);
	return json::parse(res);
}

int API::send_challenge(std::string username, bool rated, int time, int increment, std::string color, std::string variant) {
	std::string url = API_REQUEST_URL "/api/bot/challenge/" + username;
	url += "?rated=" + std::to_string(rated);
	url += "&timeControl=" + std::to_string(time) + "+" + std::to_string(increment);
	url += "&color=" + color;
	url += "&variant=" + variant;
	return _send_post(url, "", nullptr);
}

int API::accept_challenge(std::string challenge_id) {
	std::string url = API_REQUEST_URL "/api/challenge/" + challenge_id + "/accept";
	return _send_post(url, "", nullptr);
}

int API::decline_challenge(std::string challenge_id, std::string reason) {
	std::string url = API_REQUEST_URL "/api/challenge/" + challenge_id + "/decline";
	return _send_post(url, "reason=" + reason, nullptr);
}

json API::book(std::string moves) {
	std::string url = "https://explorer.lichess.ovh/masters?play=" + moves;
	std::string res;
	_send_get(url, &res);
	return json::parse(res);
}

size_t JSONStream::write_callback(char *contents, size_t size, size_t nmemb, JSONStream *obj) {
	size_t readsize = size * nmemb;

	// Remove leading newlines
	size_t realsize = readsize;
	while (contents[0] == '\n') {
		contents++;
		realsize--;
	}

	// Split messages by newlines
	while (true) {
		size_t newlinePos = std::find((char *)contents, (char *)contents + realsize, '\n') - (char *)contents;
		if (newlinePos != realsize) {
			obj->buffer.append((char *)contents, newlinePos);
			obj->msgQueue.push_back(json::parse(obj->buffer));
			obj->buffer.clear();
			if (newlinePos + 1 < realsize)
				obj->buffer.append((char *)contents + newlinePos + 1, realsize - newlinePos - 1);
			contents += newlinePos + 1;
			realsize -= newlinePos + 1;
		} else {
			obj->buffer.append((char *)contents, realsize);
			break;
		}
	}
	return readsize;
}

JSONStream::JSONStream(std::string url) {
	this->url = url;
	curl = curl_easy_init();

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Authorization: Bearer " TOKEN);
	headers = curl_slist_append(headers, "Accept: application/x-ndjson");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	t = std::thread([this]() {
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	});
}

JSONStream::~JSONStream() {
	curl_easy_cleanup(curl);
	t.join();
}

json JSONStream::waitMsg() {
	while (msgQueue.empty()) {
		if (res != CURLE_OK)
			throw std::runtime_error("Connection to " + url + " failed: " + curl_easy_strerror(res));
		if (status >= 0)
			throw std::runtime_error("Connection to " + url + " closed with status code " + std::to_string(status) + ": " + buffer);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	json msg = msgQueue.front();
	msgQueue.pop_front();
	return msg;
}
