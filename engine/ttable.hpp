#pragma once

#include "includes.hpp"
#include "move.hpp"

#define TT_SIZE (16 * 1024 * 1024 / sizeof(TTable::TTEntry))
// Note that the actual size of TT is TT_SIZE * 16 bytes

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

	TTEntry NO_ENTRY;
	TTEntry *TT;
	uint64_t tsize=0;

	TTable() { TT = new TTEntry[TT_SIZE]; }

	~TTable() { delete[] TT; }

	TTable(const TTable &o) {
		TT = new TTEntry[TT_SIZE];
		std::copy(o.TT, o.TT + TT_SIZE, TT);
		tsize = 0;
	}

	TTable &operator=(const TTable &o) {
		if (this != &o) {
			delete[] TT;
			TT = new TTEntry[TT_SIZE];
			std::copy(o.TT, o.TT + TT_SIZE, TT);
			tsize = 0;
		}
		return *this;
	}

	void store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age);

	TTEntry *probe(uint64_t key, Value alpha, Value beta, Value depth);

	uint64_t size() const;
	uint64_t mxsize() const { return TT_SIZE; }
};
