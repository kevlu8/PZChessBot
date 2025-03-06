#include "ttable.hpp"

void TTable::store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age) {
	TTEntry *entry = TT + (key & (TT_SIZE - 1));
	if (entry->flags != INVALID && entry->age > age)
		return;
	else if (entry->flags != INVALID && entry->age == age && entry->depth > depth)
		return;
	if (entry->flags == INVALID) tsize++;
	entry->key = key;
	entry->eval = eval;
	entry->depth = depth;
	entry->flags = flag;
	entry->best_move = best_move;
	entry->age = age;
}

std::pair<Value, bool> TTable::probe(uint64_t key, Value alpha, Value beta, Value depth) {
	TTEntry *entry = TT + (key & (TT_SIZE - 1));
	if (entry->key != key || entry->depth < depth)
		return {0, 0};
	if (entry->flags == EXACT) return {entry->eval, 1};
	if (entry->flags == LOWER_BOUND && entry->eval >= beta) return {entry->eval, 1};
	if (entry->flags == UPPER_BOUND && entry->eval <= alpha) return {entry->eval, 1};
	return {0, 0};
}

uint64_t TTable::size() const {
	return tsize;
}

void DrawTable::store(uint64_t key) {
	DTableEntry *entry = DT + (key & (TT_SIZE - 1));
	if (entry->key == key)
		entry->n_occ++;
	else
		*entry = DTableEntry(key);
}

uint32_t DrawTable::occ(uint64_t key) const {
	DTableEntry *entry = DT + (key & (TT_SIZE - 1));
	if (entry->key == key)
		return entry->n_occ;
	return 0;
}