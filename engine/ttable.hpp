#pragma once

#include "includes.hpp"
#include "mem.hpp"
#include "move.hpp"

#define DEFAULT_TT_SIZE (16 * 1024 * 1024 / sizeof(TTable::TTBucket)) // 16 MB
#define TT_GEN_SZ 32

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
		const bool ttpv() const { return (flags >> 2) & 1; }
		const uint8_t age() const { return flags >> 3; }
	};

	struct alignas(32) TTBucket {
		TTEntry entries[3]; // 3 * 10 = 30 bytes
		uint8_t pad[2]; // 2 bytes of padding to allow for better alignment (good compilers will do this automatically, but just to be sure)
	};

	TTEntry NO_ENTRY = TTEntry();

	TTBucket *TT;
	size_t TT_SIZE;
	uint8_t age = 0;

	void init_ttable() {
		// Multithreaded initialization (capped by thread count)
		const size_t MIN_CHUNK_SIZE = 4294967296 / sizeof(TTBucket);
		size_t num_threads = std::clamp(TT_SIZE / MIN_CHUNK_SIZE, (size_t)1UL, (size_t)std::thread::hardware_concurrency());
		std::vector<std::thread> threads;
		size_t chunk_size = TT_SIZE / num_threads;
		for (int t = 0; t < num_threads; t++) {
			threads.emplace_back([this, t, chunk_size, num_threads]() {
				size_t start = t * chunk_size;
				size_t end = (t == num_threads - 1) ? TT_SIZE : start + chunk_size;
				for (size_t i = start; i < end; i++) {
					for (int j = 0; j < 3; j++) {
						TT[i].entries[j] = TTEntry();
					}
				}
			});
		}
		for (auto &th : threads)
			th.join();
		age = 0;
	}

	void inc_gen() { age = (age + 1) % TT_GEN_SZ; }

	TTable(size_t size) : TT_SIZE(size) {
		TT = (TTBucket *)large_alloc(TT_SIZE * sizeof(TTBucket));
		init_ttable();
	}

	~TTable() { large_free(TT, TT_SIZE * sizeof(TTBucket)); }

	// TTable(const TTable &o) {
	// 	TT = new TTEntry[TT_SIZE];
	// 	std::copy(o.TT, o.TT + TT_SIZE, TT);
	// 	tsize = 0;
	// }

	TTable &operator=(const TTable &o) {
		if (this != &o) {
			large_free(TT, TT_SIZE * sizeof(TTBucket));
			TT_SIZE = o.TT_SIZE;
			TT = (TTBucket *)large_alloc(TT_SIZE * sizeof(TTBucket));
			init_ttable();
		}
		return *this;
	}

	void store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move);

	std::optional<TTEntry> probe(uint64_t key);

	void resize(size_t size) {
		if (size != TT_SIZE) {
			large_free(TT, TT_SIZE * sizeof(TTBucket));
			TT_SIZE = size;
			TT = (TTBucket *)large_alloc(TT_SIZE * sizeof(TTBucket));
		}
		init_ttable();
	}

	constexpr uint64_t mxsize() const { return TT_SIZE * 3; }
};

extern TTable ttable;
