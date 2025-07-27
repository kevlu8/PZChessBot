#include "ttable.hpp"

void TTable::store(uint64_t key, Value eval, uint8_t depth, TTFlag flag, Move best_move, uint8_t age) {
	TTBucket *bucket = TT + (key % TT_SIZE);
	
	TTEntry *depth_entry = &bucket->entries[0];
	TTEntry *age_entry = &bucket->entries[1];
	if (depth_entry->key == key || age_entry->key == key) {
		// Update an existing entry
		if (depth_entry->key == key) {
			depth_entry->eval = eval;
			depth_entry->depth = depth;
			depth_entry->flags = flag;
			depth_entry->best_move = best_move;
			depth_entry->age = age;
		} else if (age_entry->key == key) {
			age_entry->eval = eval;
			age_entry->depth = depth;
			age_entry->flags = flag;
			age_entry->best_move = best_move;
			age_entry->age = age;
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

	// 2. Check if we can replace the age entry
	if (age_entry->age < age) {
		if (age_entry->flags == INVALID) tsize++;
		age_entry->key = key;
		age_entry->eval = eval;
		age_entry->depth = depth;
		age_entry->flags = flag;
		age_entry->best_move = best_move;
		age_entry->age = age;
		return;
	}

	// boohoo, no space left
}

TTable::TTEntry *TTable::probe(uint64_t key, Value alpha, Value beta, Value depth) {
	TTBucket *bucket = TT + (key % TT_SIZE);
	for (int i = 0; i < 2; i++) {
		TTEntry *entry = &bucket->entries[i];
		if (entry->key != key || entry->depth < depth)
			continue;
		if (entry->flags == EXACT) return entry;
		if (entry->flags == LOWER_BOUND && entry->eval >= beta) return entry;
		if (entry->flags == UPPER_BOUND && entry->eval <= alpha) return entry;
	}
	return nullptr;
}
