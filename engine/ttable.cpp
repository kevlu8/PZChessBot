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

TTable::TTEntry *TTable::probe(uint64_t key) {
	TTEntry *entry = TT + (key % TT_SIZE);
	if (entry->key != key)
		return nullptr;
	return entry;
}
