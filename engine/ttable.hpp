#pragma once

#include "includes.hpp"
#include "move.hpp"

#define DEFAULT_TT_SIZE (16 * 1024 * 1024 / sizeof(TTable::TTBucket)) // 16 MB

enum TTFlag {
	EXACT = 0,
	LOWER_BOUND = 1, // eval might be higher than stored value
	UPPER_BOUND = 2, // eval might be lower than stored value
	INVALID = 3
};

struct TTable {
	struct TTEntry {
		uint64_t key;
		Move best_move;
		Value eval;
		uint8_t depth;
		uint8_t flags; // 0: exact, 1: lower bound, 2: upper, 3: empty
		uint8_t age; // half-move clock when this entry was last updated

		TTEntry() : key(0), best_move(NullMove), eval(0), depth(0), flags(INVALID), age(0) {}
		const bool valid() const { return flags != INVALID; }
		const bool operator==(const uint64_t k) const { return key == k; }
	};

	struct TTBucket {
		TTEntry entries[2];
	};

	TTBucket *TT;
	uint64_t tsize = 0;
	int TT_SIZE;

	TTable(int size) : TT_SIZE(size) { TT = new TTBucket[size]; }

	~TTable() { delete[] TT; }

	// TTable(const TTable &o) {
	// 	TT = new TTEntry[TT_SIZE];
	// 	std::copy(o.TT, o.TT + TT_SIZE, TT);
	// 	tsize = 0;
	// }

	TTable &operator=(const TTable &o) {
		if (this != &o) {
			delete[] TT;
			TT_SIZE = o.TT_SIZE;
			TT = new TTBucket[TT_SIZE];
			tsize = 0;
		}
		return *this;
	}

	void store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age);

	TTEntry *probe(uint64_t key, Value alpha=VALUE_INFINITE, Value beta=-VALUE_INFINITE, Value depth=0);

	constexpr uint64_t size() const { return tsize; }
	constexpr uint64_t mxsize() const { return TT_SIZE * 2; }
};
