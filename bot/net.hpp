#include "../token"
#include "json.hpp"
#include <curl/curl.h>
#include <deque>
#include <iostream>
#include <string>
#include <thread>

using json = nlohmann::json;

class JSONStream {
private:
	std::string url;
	CURL *curl;
	std::string buffer;
	std::deque<json> msgQueue;
	std::thread t;
	bool active;

	static size_t write_callback(char *, size_t, size_t, JSONStream *);

public:
	JSONStream(std::string url);
	~JSONStream();

	// Blocks until a message is available, or returns an empty json object if the stream is closed
	json waitMsg();
};
