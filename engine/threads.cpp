/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

#include "threads.hpp"

#ifdef USE_NUMA
#include <numa.h>
#endif

void Pool::resize(size_t num) {
	if (num == num_threads)
		return;

	std::unique_lock lock(mtx);

	stop = true;
	start_barrier->arrive_and_wait();
	for (auto &t : threads) {
		t.join();
	}
	threads.clear();
	for (size_t i = 0; i < num_threads; ++i) {
		tis[i].~ThreadInfo();
	}
	large_free(tis, num_threads * sizeof(ThreadInfo));

	num_threads = num;
	stop = false;
	tis = (ThreadInfo *)large_alloc(num_threads * sizeof(ThreadInfo));
	start_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
	ready_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
	init_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
	for (size_t i = 0; i < num_threads; ++i) {
		threads.emplace_back(&Pool::thread_loop, this, i);
	}
	init_barrier->arrive_and_wait();
}

void Pool::thread_loop(size_t i) {
	int node = 0;
#ifdef USE_NUMA
	if (num_threads >= numa_num_configured_nodes()) {
		node = i % numa_num_configured_nodes();
		numa_run_on_node(node);
		sched_yield();
	}
#endif
	new (&tis[i]) ThreadInfo(get_network(node)); // construct in thread loop for better NUMA locality
	init_barrier->arrive_and_wait();
	while (true) {
		start_barrier->arrive_and_wait();
		if (stop)
			break;
		if (i == 0)
			ttable.inc_gen();
		{
			std::shared_lock lock(mtx);
			ready_barrier->arrive_and_wait();

			iterativedeepening(pos, tis[i], depth);
		}
	}
}

void Pool::search(Position &pos, RepetitionHandler &rp, int64_t time, int depth, int64_t maxnodes, bool quiet) {
	prepare_search(time, maxnodes, quiet, num_threads);
	this->depth = depth;
	this->pos = pos;

	for (int t = 0; t < num_threads; t++) {
		ThreadInfo &ti = tis[t];
		ti.rp = rp;
		ti.am.full_refresh(pos, 0);
		ti.seldepth = 0;
		nodes[t] = 0;
		ti.id = t;
		ti.is_main = (t == 0);
	}

	bool rep = false;
	for (int i = rp.hash_hist.size() - 2; i >= 0; i--) {
		if (rp.hash_hist[i] == pos.zobrist_without_ep()) {
			rep = true;
			break;
		}
	}
	tb_moves = tbman.probe_moves(pos, rep);

	start_barrier->arrive_and_wait();
	ready_barrier->arrive_and_wait();
}

std::pair<Move, Value> Pool::wait_finished() {
	std::unique_lock lock(mtx);

	ThreadInfo &best_thread = tis[0];
	return {best_thread.pvtable[0][0], best_thread.eval};
}
