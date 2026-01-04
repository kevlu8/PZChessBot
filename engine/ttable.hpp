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
	struct TTEntry { // 2 + 2 + 4 + 1 + 1 + 1 = 10 bytes
		uint16_t key; // 2 bytes
		Move best_move; // 2 bytes
		Value eval, s_eval; // 2 + 2 bytes
		uint8_t depth; // 1 byte
		uint8_t flags; // 0: exact, 1: lower bound, 2: upper, 3: empty - 1 byte

		TTEntry() : key(0), best_move(NullMove), eval(0), s_eval(0), depth(0), flags(NONE) {}
		const bool valid() const { return flags != NONE; }
		const TTFlag bound() const { return TTFlag(flags & 3); }
		const bool ttpv() const { return flags >> 2; }
	};

	struct alignas(32) TTBucket {
		TTEntry entries[3]; // 3 * 10 = 30 bytes
		uint8_t pad[2]; // 2 bytes of padding to allow for better alignment (good compilers will do this automatically, but just to be sure)
	};

	TTEntry NO_ENTRY = TTEntry();

	TTBucket *TT;
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
		}
		return *this;
	}

	void store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move);

	TTEntry *probe(uint64_t key);

	void resize(int size) {
		delete[] TT;
		TT_SIZE = size;
		TT = new TTBucket[size];
	}

	constexpr uint64_t mxsize() const { return TT_SIZE * 2; }
};

extern TTable ttable;
