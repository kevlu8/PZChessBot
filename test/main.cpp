#define DOCTEST_CONFIG_IMPLEMENT
#include "test.hpp"

std::mt19937_64 test_rng(0x2173736568435a50);

int main() {
	doctest::Context context;

	// context.setOption("success", true);
	context.setOption("duration", true);
	context.setOption("no-breaks", true);

	int res = context.run();
	if (context.shouldExit())
		return res;

	return 0;
}
