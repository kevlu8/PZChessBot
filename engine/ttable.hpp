#pragma once

#include "includes.hpp"
#include "bitboard.hpp"

#define TT_SIZE (1 << 24)

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

		constexpr TTEntry() : key(0), best_move(NullMove), eval(0), depth(0), flags(INVALID), age(0) {}
		const bool valid() const { return flags != INVALID; }
		const bool operator==(const uint64_t k) const { return key == k; }
	};

	TTEntry NO_ENTRY;
	TTEntry *TT;
	uint64_t tsize;

	void init();

	void store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age);

	TTEntry *probe(uint64_t key);

	constexpr uint64_t size() const;
	constexpr uint64_t mxsize() const { return TT_SIZE; }
};

struct DrawTable {
	struct DTableEntry {
		uint64_t key;
		uint32_t n_occ;

		constexpr DTableEntry() : key(0), n_occ(0) {}
		constexpr DTableEntry(uint64_t k) : key(k), n_occ(1) {}
	};

	DTableEntry *DT;

	void init();

	void store(uint64_t key);

	uint32_t occ(uint64_t key) const;
};
