#include "ttable.hpp"

TTable ttable(DEFAULT_TT_SIZE);

void TTable::store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move, uint8_t age) {
	TTEntry *entry = TT + (key % TT_SIZE);

	key >>= 32; // Use upper 32 bits for the key (since we already verified bottom n bits)

	// Same key -> update if depth is greater or equal
	if (entry->key == key && depth >= entry->depth) {
		entry->best_move = best_move;
		entry->eval = eval;
		entry->s_eval = s_eval;
		entry->depth = depth;
		entry->flags = bound | (ttpv ? TTPV : 0);
		entry->age = age;
		return;
	}

	// Replace if !(depth == 0 and entry exists and entry depth > 0)
	// i.e. do not replace good entries with static eval-only entries
	if (!(depth == 0 && entry->valid() && entry->depth > 0)) {
		// New entry
		entry->key = key;
		entry->best_move = best_move;
		entry->eval = eval;
		entry->s_eval = s_eval;
		entry->depth = depth;
		entry->flags = bound | (ttpv ? TTPV : 0);
		entry->age = age;
	}
}

TTable::TTEntry *TTable::probe(uint64_t key) {
	TTEntry *entry = TT + (key % TT_SIZE);
	key >>= 32;
	if (entry->key == key)
		return entry;
	return nullptr;
}
