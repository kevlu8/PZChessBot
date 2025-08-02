#pragma once
#ifndef TEST_HPP
#define TEST_HPP

#include "doctest.h"

#include <random>

extern std::mt19937_64 test_rng;

#define _SHORT(v, c, e, __VA_ARGS__...) \
	c(__VA_ARGS__);                     \
	if (!(e))                           \
		v = false;

#define SHORT(v, e) \
	_SHORT(v, CHECK, e, e);

#define SHORT_EQ(v, a, b) \
	_SHORT(v, CHECK_EQ, (a) == (b), a, b)

#define SHORT_NE(v, a, b) \
	_SHORT(v, CHECK_NE, (a) != (b), a, b)

#endif
