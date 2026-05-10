#include "threads.hpp"

#include <cassert>

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
	for (size_t i = 0; i < num_threads; ++i) {
		new (&tis[i]) ThreadInfo();
		threads.emplace_back(&Pool::thread_loop, this, i);
	}
}

void Pool::thread_loop(size_t i) {
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
		ti.am = AccumulatorManager(pos);
		ti.seldepth = 0;
		nodes[t] = 0;
		ti.id = t;
		ti.is_main = (t == 0);
	}

	bool rep = false;
	assert(rp.hash_hist.empty() || rp.hash_hist[rp.hash_hist.size() - 1] == pos.zobrist);
	for (int i = rp.hash_hist.size() - 2; i >= 0; i--) {
		if (rp.hash_hist[i] == pos.zobrist) {
			rep = true;
			break;
		}
	}
	tb_moves = tbman.probe_moves(pos, rep);

	if (!tb_moves.empty()) {
		std::cerr << "Tablebase hit! Viable moves: ";
		for (auto move : tb_moves) {
			std::cerr << Move(move).to_string() << " ";
		}
		std::cerr << std::endl;
	}

	start_barrier->arrive_and_wait();
	ready_barrier->arrive_and_wait();
}

std::pair<Move, Value> Pool::wait_finished() {
	std::unique_lock lock(mtx);

	ThreadInfo &best_thread = tis[0];
	return {best_thread.pvtable[0][0], best_thread.eval};
}
