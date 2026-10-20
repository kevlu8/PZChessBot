/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

// Compare lazy incremental accumulators against a complete rebuild, including
// skipped evaluations, sibling searches, mirror changes and special moves.
#include "../engine/eval.hpp"
#include "../engine/movegen.hpp"
#include <deque>
#include <memory>
#include <random>
#include <stdexcept>

bool dfrc_uci = false;

void verify(Position &pos, AccumulatorManager &incremental, AccumulatorManager &reference) {
	incremental.apply_lazy(pos);
	reference.full_refresh(pos, 0);
	auto &a = incremental.current();
	auto &b = reference.current();
	if (memcmp(a.w_acc.val, b.w_acc.val, sizeof(a.w_acc.val)) ||
		memcmp(a.b_acc.val, b.b_acc.val, sizeof(a.b_acc.val)) ||
		memcmp(a.w_threats.val, b.w_threats.val, sizeof(a.w_threats.val)) ||
		memcmp(a.b_threats.val, b.b_threats.val, sizeof(a.b_threats.val)))
		throw std::runtime_error("Accumulator mismatch: " + pos.get_fen());
	if (eval(pos, incremental) != eval(pos, reference))
		throw std::runtime_error("Evaluation mismatch: " + pos.get_fen());
}

void verify_threat_indices() {
	int bases[14] = {}, sizes[14] = {}, offsets[14][64] = {};
	int total = 0;
	for (int piece = 0; piece < 14; piece++) {
		if (piece == 6 || piece == 7) continue;
		bases[piece] = total;
		for (int src = 0; src < 64; src++) {
			offsets[piece][src] = sizes[piece];
			if ((piece & 7) == PAWN && (src < SQ_A2 || src > SQ_H7)) continue;
			sizes[piece] += arch::popcnt(calc_attacks(Piece(piece), Square(src), 0));
		}
		total += sizes[piece] * NTARGETS[piece & 7] * 2;
	}
	int checked = 0;
	for (int perspective = 0; perspective < 2; perspective++) {
		for (int mirror = 0; mirror < 2; mirror++) {
			for (int attacker = 0; attacker < 14; attacker++) {
				if (attacker == 6 || attacker == 7) continue;
				for (int src = 0; src < 64; src++) {
					if ((attacker & 7) == PAWN && (src < SQ_A2 || src > SQ_H7)) continue;
					Bitboard attacks = calc_attacks(Piece(attacker), Square(src), 0);
					while (attacks) {
						int dst = arch::tzcnt(attacks);
						attacks = arch::blsr(attacks);
						for (int victim = 0; victim < 14; victim++) {
							if (victim == 6 || victim == 7) continue;
							int a = attacker ^ (perspective * 8), v = victim ^ (perspective * 8);
							int from = src ^ (perspective * 56) ^ (mirror * 7);
							int to = dst ^ (perspective * 56) ^ (mirror * 7);
							int target = TARGETS[a & 7][v & 7];
							bool semi_excluded = (a & 7) == (v & 7) && ((a >> 3) != (v >> 3) || (a & 7) != PAWN);
							int expected = -1;
							if (target >= 0 && !(semi_excluded && from < to)) {
								Bitboard lower = calc_attacks(Piece(a), Square(from), 0) & (square_bits(Square(to)) - 1);
								expected = bases[a] + ((v >> 3) * NTARGETS[a & 7] + target) * sizes[a]
									+ offsets[a][from] + arch::popcnt(lower);
							}
							int actual = threat_index(perspective, mirror ? SQ_E4 : SQ_D4,
								Piece(attacker), Piece(victim), Square(src), Square(dst));
							if (actual != expected) throw std::runtime_error("Threat index mismatch");
							checked++;
						}
					}
				}
			}
		}
	}
	std::cout << "Verified " << checked << " threat feature indices\n";
}

int main() {
	verify_threat_indices();
	const char *positions[] = {
		"startpos",
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
		"4k3/P6p/8/3pP3/8/8/p6P/4K3 w - d6 0 1",
		"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
		"4k3/8/8/8/8/8/8/R2K3R w HA - 0 1",
		"4k3/8/8/8/8/8/8/5KR1 w G - 0 1",
		"4k3/8/8/8/8/8/8/6KR w H - 0 1",
		"4k3/8/8/8/8/8/8/2RK4 w C - 0 1",
	};
	std::mt19937 rng(0x507a);
	int verified = 0;
	for (auto fen : positions) {
		for (int game = 0; game < 20; game++) {
			dfrc_uci = false;
			std::deque<Position> boards;
			boards.emplace_back(fen);
			auto incremental = std::make_unique<AccumulatorManager>(boards.back());
			auto reference = std::make_unique<AccumulatorManager>(boards.back());
			for (int ply = 0; ply < 100; ply++) {
				Position &before = boards.back();
				pzstd::vector<Move> moves;
				before.legal_moves(moves);
				pzstd::vector<Move> legal;
				for (Move move : moves)
					if (before.is_legal(move)) legal.push_back(move);
				if (legal.empty()) break;
				// Probe every sibling periodically, including all root special moves.
				if (ply % 11 == 0) {
					for (Move move : legal) {
						Position after = before;
						after.make_move(move);
						incremental->make_move(before, move, after);
						verify(after, *incremental, *reference);
						verified++;
						incremental->pop_move();
					}
				}
				Move move = legal[rng() % legal.size()];
				boards.push_back(before);
				boards.back().make_move(move);
				incremental->make_move(before, move, boards.back());
				if (rng() % 4 == 0) {
					verify(boards.back(), *incremental, *reference);
					verified++;
				}
			}
			verify(boards.back(), *incremental, *reference);
			while (boards.size() > 1) {
				incremental->pop_move();
				boards.pop_back();
				if (rng() % 3 == 0) {
					verify(boards.back(), *incremental, *reference);
					verified++;
				}
			}
		}
	}
	std::cout << "Verified " << verified << " incremental accumulator positions\n";
}
