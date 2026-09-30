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

#include <atomic>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <pthread.h>
#endif

#ifdef USE_NUMA
#include <numa.h>
#endif

void raise_thread_stack_size() {
#ifdef __linux__
	constexpr size_t STACK_SIZE = 8 * 1024 * 1024; // 8 MB
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, STACK_SIZE);
	pthread_setattr_default_np(&attr);
	pthread_attr_destroy(&attr);
#endif
}

static int64_t node_ticket = -1;
uint32_t get_node_ticket() {
#if defined(_WIN32)
	// no thanks, someone else can come do this if they want
	return 0;
#else
	if (node_ticket != -1)
		return node_ticket;

	int fd = shm_open("/pzsync", O_CREAT | O_RDWR, 0666);
	if (fd < 0)
		return 0;

	if (ftruncate(fd, 4) < 0) {
		close(fd);
		return 0;
	}

	void *ptr = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0);
	close(fd);
	if (ptr == MAP_FAILED)
		return 0;

	node_ticket = reinterpret_cast<std::atomic<uint32_t> *>(ptr)->fetch_add(1, std::memory_order_relaxed);
	munmap(ptr, 4);
	return node_ticket;
#endif
}

void Pool::resize(size_t num) {
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

	init_networks(multiInstance);

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
	if (testing_mode) {
		node = get_node_ticket() % numa_num_configured_nodes();
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
