#include "ttable.hpp"

TTable ttable[MAX_THREADS] = { TTable() };

void TTable::store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move, uint8_t age) {
	TTBucket *bucket = TT + (key % TT_SIZE);

	key >>= 32; // Use upper 32 bits for the key (since we already verified bottom n bits)
	
	TTEntry *depth_entry = &bucket->entries[0];
	TTEntry *always_entry = &bucket->entries[1];
	if (depth_entry->key == key || always_entry->key == key) {
		// Update an existing entry
		if (depth_entry->key == key) {
			depth_entry->eval = eval;
			depth_entry->s_eval = s_eval;
			depth_entry->depth = depth;
			depth_entry->flags = bound | (ttpv ? TTPV : 0);
			depth_entry->best_move = best_move;
			depth_entry->age = age;
		} else if (always_entry->key == key) {
			always_entry->eval = eval;
			always_entry->s_eval = s_eval;
			always_entry->depth = depth;
			always_entry->flags = bound | (ttpv ? TTPV : 0);
			always_entry->best_move = best_move;
			always_entry->age = age;
		}
		return;
	}

	// 1. Check if we can replace the depth entry
	if (depth_entry->depth < depth || (depth_entry->depth == depth && depth_entry->age < age)) {
		depth_entry->key = key;
		depth_entry->eval = eval;
		depth_entry->s_eval = s_eval;
		depth_entry->depth = depth;
		depth_entry->flags = bound | (ttpv ? TTPV : 0);
		depth_entry->best_move = best_move;
		depth_entry->age = age;
		return;
	}

	// 2. Always replace the second entry
	always_entry->key = key;
	always_entry->eval = eval;
	always_entry->s_eval = s_eval;
	always_entry->depth = depth;
	always_entry->flags = bound | (ttpv ? TTPV : 0);
	always_entry->best_move = best_move;
	always_entry->age = age;
}

TTable::TTEntry *TTable::probe(uint64_t key) {
	TTBucket *bucket = TT + (key % TT_SIZE);
	key >>= 32;
	for (int i = 0; i < 2; i++) {
		TTEntry *entry = &bucket->entries[i];
		if (entry->key != key)
			continue;
		return entry;
	}
	return nullptr;
}
