#pragma once

#include "includes.hpp"
#include "move.hpp"

#define DEFAULT_TT_SIZE (16 * 1024 * 1024 / sizeof(TTable::TTBucket)) // 16 MB

enum TTFlag {
	EXACT = 0,
	LOWER_BOUND = 1, // eval might be higher than stored value
	UPPER_BOUND = 2, // eval might be lower than stored value
	NONE = 3,
	TTPV = 4
};

struct TTable {
	struct TTEntry {
		uint32_t key;
		Move best_move;
		Value eval, s_eval;
		uint8_t depth;
		uint8_t flags; // 0: exact, 1: lower bound, 2: upper, 3: empty
		uint8_t age; // half-move clock when this entry was last updated

		TTEntry() : key(0), best_move(NullMove), eval(0), s_eval(0), depth(0), flags(NONE), age(0) {}
		const bool valid() const { return flags != NONE; }
		const TTFlag bound() const { return TTFlag(flags & 3); }
		const bool ttpv() const { return flags >> 2; }
	};

	struct TTBucket {
		TTEntry entries[2];
	};

	TTBucket *TT;
	int TT_SIZE;

	TTable() : TT_SIZE(DEFAULT_TT_SIZE) { TT = new TTBucket[DEFAULT_TT_SIZE]; }
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
		}
		return *this;
	}

	void store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move, uint8_t age);

	TTEntry *probe(uint64_t key);

	void clear() {
		memset(TT, 0, sizeof(TTBucket) * TT_SIZE);
	}

	constexpr uint64_t mxsize() const { return TT_SIZE * 2; }
};

extern TTable ttable[MAX_THREADS * 2];
