#include "ttable.hpp"

TTable ttable(DEFAULT_TT_SIZE);

void TTable::store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move, uint8_t age) {
	TTEntry *entry = TT + (key % TT_SIZE);

	key >>= 32; // Use upper 32 bits for the key (since we already verified bottom n bits)

	entry->key = key;
	entry->eval = eval;
	entry->s_eval = s_eval;
	entry->depth = depth;
	entry->flags = bound | (ttpv ? TTPV : 0);
	entry->best_move = best_move;
	entry->age = age;
}

const TTable::TTEntry *TTable::probe(uint64_t key) {
	TTEntry *entry = TT + (key % TT_SIZE);
	key >>= 32;
	if (entry->key == key)
		return entry;
	return &NO_ENTRY;
}
