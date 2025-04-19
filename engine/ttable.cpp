#include "ttable.hpp"

void TTable::store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age) {
	TTEntry *entry = TT + (key % TT_SIZE);
	if (entry->flags == INVALID) tsize++;
	if (entry->key != key && entry->depth > depth) {
		// This entry contains more information than the new one
		// So we don't overwrite it
		return;
	}
	entry->key = key;
	entry->eval = eval;
	entry->depth = depth;
	entry->flags = flag;
	entry->best_move = best_move;
	entry->age = age;
}

TTable::TTEntry *TTable::probe(uint64_t key, Value alpha, Value beta, Value depth) {
	TTEntry *entry = TT + (key % TT_SIZE);
	if (entry->key != key || entry->depth < depth)
		return nullptr;
	if (entry->flags == EXACT) return entry;
	if (entry->flags == LOWER_BOUND && entry->eval >= beta) return entry;
	if (entry->flags == UPPER_BOUND && entry->eval <= alpha) return entry;
	return nullptr;
}
