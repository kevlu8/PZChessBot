#pragma once

#include "includes.hpp"
#include "search.hpp"

#include <barrier>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

class Pool {
private:
	size_t num_threads;
	std::vector<std::thread> threads;
	ThreadInfo *tis;

	std::unique_ptr<std::barrier<>> start_barrier, ready_barrier;
	std::shared_mutex mtx;
	bool stop;

	int depth;

	void thread_loop(size_t i);

public:
	Pool() : num_threads(1), stop(false) {
		tis = (ThreadInfo *)large_alloc(num_threads * sizeof(ThreadInfo));
		new (tis) ThreadInfo();
		start_barrier = std::make_unique<std::barrier<>>(2);
		ready_barrier = std::make_unique<std::barrier<>>(2);
		threads.emplace_back(&Pool::thread_loop, this, 0);
	}

	Pool(size_t num_threads) : num_threads(num_threads), stop(false) {
		tis = (ThreadInfo *)large_alloc(num_threads * sizeof(ThreadInfo));
		start_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
		ready_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
		for (size_t i = 0; i < num_threads; ++i) {
			new (&tis[i]) ThreadInfo();
			threads.emplace_back(&Pool::thread_loop, this, i);
		}
	}

	void resize(size_t num);

	void search(Board &board, int64_t time, int depth, int64_t maxnodes, bool quiet);

	void clear_search_vars() {
		std::unique_lock lock(mtx);
		for (size_t i = 0; i < num_threads; i++) {
			::clear_search_vars(tis[i]);
		}
	}

	ThreadInfo &get_ti(size_t i) {
		return tis[i];
	}

	std::pair<Move, Value> wait_finished();

	~Pool() {
		stop = true;
		start_barrier->arrive_and_wait();
		for (auto &t : threads) {
			t.join();
		}
		for (size_t i = 0; i < num_threads; ++i) {
			tis[i].~ThreadInfo();
		}
		large_free(tis, num_threads * sizeof(ThreadInfo));
	}
};
