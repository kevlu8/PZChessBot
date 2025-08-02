#include "ttable.hpp"

void TTable::store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age) {
	TTBucket *bucket = TT + (key % TT_SIZE);
	
	TTEntry *depth_entry = &bucket->entries[0];
	TTEntry *always_entry = &bucket->entries[1];
	if (depth_entry->key == key || always_entry->key == key) {
		// Update an existing entry
		if (depth_entry->key == key) {
			depth_entry->eval = eval;
			depth_entry->depth = depth;
			depth_entry->flags = flag;
			depth_entry->best_move = best_move;
			depth_entry->age = age;
		} else if (always_entry->key == key) {
			always_entry->eval = eval;
			always_entry->depth = depth;
			always_entry->flags = flag;
			always_entry->best_move = best_move;
			always_entry->age = age;
		}
		return;
	}

	// 1. Check if we can replace the depth entry
	if (depth_entry->depth < depth || (depth_entry->depth == depth && depth_entry->age < age)) {
		if (depth_entry->flags == INVALID) tsize++;
		depth_entry->key = key;
		depth_entry->eval = eval;
		depth_entry->depth = depth;
		depth_entry->flags = flag;
		depth_entry->best_move = best_move;
		depth_entry->age = age;
		return;
	}

	// 2. Always replace the second entry
	if (always_entry->flags == INVALID) tsize++;
	always_entry->key = key;
	always_entry->eval = eval;
	always_entry->depth = depth;
	always_entry->flags = flag;
	always_entry->best_move = best_move;
	always_entry->age = age;
}

TTable::TTEntry *TTable::probe(uint64_t key, Value depth) {
	TTBucket *bucket = TT + (key % TT_SIZE);
	for (int i = 0; i < 2; i++) {
		TTEntry *entry = &bucket->entries[i];
		if (entry->key != key || entry->depth < depth)
			continue;
		return entry;
	}
	return nullptr;
}
