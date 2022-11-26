#include "api.hpp"

int main() {
	// this is just for now
	std::string g;
	std::cin >> g;
	std::string s;
	while (std::cin >> s) {
		API::move(g, s);
	}
	return 0;
}