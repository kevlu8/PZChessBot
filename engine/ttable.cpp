#include "ttable.hpp"

void TTable::init() {
	TT = new TTEntry[TT_SIZE];
}

void TTable::store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age) {
	TTEntry *entry = TT + (key & (TT_SIZE - 1));
	if (entry->age < age)
		return;
	else if (entry->age == age && entry->depth > depth)
		return;
	if (entry->flags == INVALID) tsize++;
	entry->key = key;
	entry->eval = eval;
	entry->depth = depth;
	entry->flags = flag;
	entry->best_move = best_move;
	entry->age = age;
}

TTable::TTEntry *TTable::probe(uint64_t key) {
	TTEntry *entry = TT + (key & (TT_SIZE - 1));
	if (entry->key == key)
		return entry;
	return &NO_ENTRY;
}

constexpr uint64_t TTable::size() const {
	return tsize;
}

void DrawTable::init() {
	DT = new DTableEntry[TT_SIZE];
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