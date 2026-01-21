#include "ttable.hpp"

TTable ttable(DEFAULT_TT_SIZE);

void TTable::store(uint64_t key, Value eval, Value s_eval, uint8_t depth, uint8_t bound, bool ttpv, Move best_move) {
	TTBucket bucket = TT[key % TT_SIZE].load();

	uint16_t smallkey = key >> 48; // Use upper 16 bits for the key (since we already verified bottom n bits)

	TTEntry entry = bucket.entries[0];
	uint8_t idx = 0;

	if (entry.key != 0 && entry.key != smallkey) {
		for (uint8_t i = 1; i < 3; i++) {
			const TTEntry nentry = bucket.entries[i];
			if (nentry.key == smallkey) {
				entry = nentry;
				idx = i;
				break;
			}

			if (nentry.depth < entry.depth) {
				entry = nentry;
				idx = i;
			}
		}
	}

	if (best_move == NullMove && entry.key == smallkey)
		best_move = entry.best_move; // Preserve best move if none given

	uint8_t existing_flag_bonus = 3 - (uint8_t)entry.bound(); // 0 = exact, 1 = lower, 2 = upper, 3 = none
	uint8_t new_flag_bonus = 3 - bound;

	uint16_t existing_prio = entry.depth + existing_flag_bonus;
	uint16_t new_prio = depth + new_flag_bonus;

	if (entry.key != smallkey || (bound == EXACT && entry.bound() != EXACT) || new_prio * 3 >= existing_prio * 2) {
		entry.key = smallkey;
		entry.eval = eval;
		entry.s_eval = s_eval;
		entry.depth = depth;
		entry.flags = bound | (ttpv ? TTPV : 0);
		entry.best_move = best_move;

		bucket.entries[idx] = entry;
	}

	TT[key % TT_SIZE].store(bucket);
}

std::optional<TTable::TTEntry> TTable::probe(uint64_t key) {
	TTBucket bucket = TT[key % TT_SIZE].load();
	key >>= 48;
	for (int i = 0; i < 3; i++) {
		if (bucket.entries[i].key != key)
			continue;
		return bucket.entries[i];
	}
	return {};
}
