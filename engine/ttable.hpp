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

		TTEntry() : key(0), best_move(NullMove), eval(-VALUE_INFINITE), s_eval(0), depth(0), flags(NONE) {}
		const bool valid() const { return flags != NONE; }
		const TTFlag bound() const { return TTFlag(flags & 3); }
		const bool ttpv() const { return flags >> 2; }
	};

	struct alignas(32) TTBucket {
		TTEntry entries[3]; // 3 * 10 = 30 bytes
		uint8_t pad[2]; // 2 bytes of padding to allow for better alignment (good compilers will do this automatically, but just to be sure)
	};

	struct internal_TTMEMORY {
		alignas(32) std::atomic<uint64_t> data[4] = {};

		internal_TTMEMORY() {
			TTBucket empty = TTBucket();
			std::memcpy(data, &empty, sizeof(TTBucket));
		}

		TTBucket load() {
			TTBucket bucket;
			uint64_t d[4];
			d[0] = data[0].load(std::memory_order_relaxed);
			d[1] = data[1].load(std::memory_order_relaxed);
			d[2] = data[2].load(std::memory_order_relaxed);
			d[3] = data[3].load(std::memory_order_relaxed);
			std::memcpy(&bucket, d, sizeof(TTBucket));
			return bucket;
		}

		void store(TTBucket &bucket) {
			uint64_t d[4];
			std::memcpy(d, &bucket, sizeof(TTBucket));
			data[0].store(d[0], std::memory_order_relaxed);
			data[1].store(d[1], std::memory_order_relaxed);
			data[2].store(d[2], std::memory_order_relaxed);
			data[3].store(d[3], std::memory_order_relaxed);
		}
	};

	TTEntry NO_ENTRY = TTEntry();

	internal_TTMEMORY *TT;
	int TT_SIZE;

	TTable(int size) : TT_SIZE(size) { TT = new internal_TTMEMORY[size]; }

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
			TT = new internal_TTMEMORY[TT_SIZE];
		}
		return *this;
	}

	void store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move);

	std::optional<TTEntry> probe(uint64_t key);

	void resize(int size) {
		delete[] TT;
		TT_SIZE = size;
		TT = new internal_TTMEMORY[size];
	}

	constexpr uint64_t mxsize() const { return TT_SIZE * 3; }
};

extern TTable ttable;
