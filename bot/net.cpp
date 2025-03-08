#include "net.hpp"

size_t JSONStream::write_callback(char *contents, size_t size, size_t nmemb, JSONStream *obj) {
	size_t realsize = size * nmemb;
	if (realsize && contents[0] == '\n')
		return realsize;
	size_t newlinePos = std::find((char *)contents, (char *)contents + realsize, '\n') - (char *)contents;
	if (newlinePos != realsize) {
		obj->buffer.append((char *)contents, newlinePos);
		obj->msgQueue.push_back(json::parse(obj->buffer));
		obj->buffer.clear();
		if (newlinePos + 1 < realsize)
			obj->buffer.append((char *)contents + newlinePos + 1, realsize - newlinePos - 1);
	} else {
		obj->buffer.append((char *)contents, realsize);
	}
	return realsize;
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
		CURLcode e = curl_easy_perform(curl);
		if (e != CURLE_OK)
			throw std::runtime_error("Failed to establish stream to " + this->url + ": " + curl_easy_strerror(e) + '\n');
		active = false;
	});
}

JSONStream::~JSONStream() {
	curl_easy_cleanup(curl);
	t.join();
}

json JSONStream::waitMsg() {
	while (msgQueue.empty()) {
		if (!active)
			return json();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	json msg = msgQueue.front();
	msgQueue.pop_front();
	return msg;
}
