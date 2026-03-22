#include "threads.hpp"

#include <numa.h>

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
	delete tis;

	num_threads = num;
	stop = false;
	tis = new ThreadInfo *[num_threads];
	start_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
	ready_barrier = std::make_unique<std::barrier<>>(num_threads + 1);
	for (size_t i = 0; i < num_threads; ++i) {
		threads.emplace_back(&Pool::thread_loop, this, i);
	}
	ready_barrier->arrive_and_wait();
}

void Pool::thread_loop(size_t i) {
#if defined(__linux__)
	ThreadInfo *local_ti;
	if (numa_available() != -1) {
		int num_nodes = numa_max_node() + 1;
		int node = i % num_nodes;
		numa_run_on_node(node);
		numa_set_preferred(node);
		local_ti = (ThreadInfo *)numa_alloc_onnode(sizeof(ThreadInfo), node);
		new (local_ti) ThreadInfo();
	} else {
		local_ti = new ThreadInfo();
	}
	sched_param sch_params = {.sched_priority = 99};
	sched_setscheduler(0, SCHED_FIFO, &sch_params);
	sched_yield();
#elif defined(_WIN32)
	local_ti = new ThreadInfo();
#endif

	tis[i] = local_ti;
	ready_barrier->arrive_and_wait();

	while (true) {
		start_barrier->arrive_and_wait();
		if (stop)
			break;
		if (i == 0)
			ttable.inc_gen();
		{
			std::shared_lock lock(mtx);
			ready_barrier->arrive_and_wait();

			iterativedeepening(pos, *local_ti, depth);
		}
	}

#if defined(__linux__)
	if (numa_available() != -1) {
		local_ti->~ThreadInfo();
		numa_free(local_ti, sizeof(ThreadInfo));
	} else {
		delete local_ti;
	}
#elif defined(_WIN32)
	delete local_ti;
#endif
}

void Pool::search(Position &pos, RepetitionHandler &rp, int64_t time, int depth, int64_t maxnodes, bool quiet) {
	prepare_search(time, maxnodes, quiet, num_threads);
	this->depth = depth;
	this->pos = pos;

	for (int t = 0; t < num_threads; t++) {
		ThreadInfo *ti = tis[t];
		ti->rp = rp;
		ti->am = AccumulatorManager(pos);
		ti->seldepth = 0;
		nodes[t] = 0;
		ti->id = t;
		ti->is_main = (t == 0);
	}

	start_barrier->arrive_and_wait();
	ready_barrier->arrive_and_wait();
}

std::pair<Move, Value> Pool::wait_finished() {
	std::unique_lock lock(mtx);

	ThreadInfo *best_thread = tis[0];
	return {best_thread->pvtable[0][0], best_thread->eval};
}
