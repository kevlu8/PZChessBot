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

#include "threats.hpp"

int PIECE_THREAT_INDEX[14][64][64];

__attribute__((constructor(106))) void init_piece_threat_index() {
	for (int p = 0; p < 14; p++) {
		for (int src = 0; src < 64; src++) {
			for (int dst = 0; dst < 64; dst++) {
				PIECE_THREAT_INDEX[p][src][dst] = -1;
			}
		}
	}
	
	for (int pid = 0; pid < 14; pid++) {
		if (pid == 6 || pid == 7) continue; // undefined pieces

		Piece p = (Piece)pid;
		for (int src = 0; src < 64; src++) {
			Bitboard attacks = calc_attacks(p, (Square)src, 0);
			
			for (int dst = 0; dst < 64; dst++) {
				// attack id is ordered by square number, so the lowest square id (e.g. A1 = 0) would be id 0
				// to easily do this, for each square dst we can make a mask of all squares < dst then AND w/ attacks then popcount
				Bitboard mask = (1ull << dst) - 1;
				PIECE_THREAT_INDEX[pid][src][dst] = arch::popcnt(attacks & mask);
			}
		}
	}
}

int PIECE_OFFSET[14];
int TOTAL_OFFSET[14];
int OFFSETS[14][64];

__attribute__((constructor(107))) void init_offsets() {
	int g_offset = 0;

	for (int pid = 0; pid < 14; pid++) {
		if (pid == 6 || pid == 7) continue; // undefined pieces

		Piece p = (Piece)pid;
		int total = 0;
		for (int sq = 0; sq < 64; sq++) {
			OFFSETS[pid][sq] = total;
			if (PieceType(p & 7) == PAWN && (sq < SQ_A2 || sq > SQ_H7))
				continue; // pawns can't attack from the first or last rank
			int count = arch::popcnt(calc_attacks(p, (Square)sq, 0));
			total += count;
		}

		PIECE_OFFSET[pid] = total;
		TOTAL_OFFSET[pid] = g_offset;
		g_offset += total * NTARGETS[PieceType(p & 7)] * 2;
	}

	// at this point, g_offset should be NTHREATS (total num threats)
	if (g_offset != NTHREATS) {
		throw std::runtime_error("je vais te menacer");
	}
}

int ATTACK_INDEX[14][14][2];

__attribute__((constructor(108))) void init_attack_index() {
	for (int attacker = 0; attacker < 14; attacker++) {
		if (attacker == 6 || attacker == 7) continue; // undefined pieces

		Piece atk = (Piece)attacker;
		for (int victim = 0; victim < 14; victim++) {
			if (victim == 6 || victim == 7) continue; // undefined pieces

			Piece vic = (Piece)victim;
			bool opposite = (attacker >> 3) != (victim >> 3);
			int target = TARGETS[atk & 7][vic & 7];

			// for pieces, same piecetypes are semiexcluded. for pawns, opposite colors are semiexcluded
			bool semi_excl = (atk & 7) == (vic & 7) && (opposite || (atk & 7) != PAWN);
			bool excl = target == -1;

			int p_offset = PIECE_OFFSET[attacker]; // total attacks of piece attacker
			int t_offset = TOTAL_OFFSET[attacker]; // global offset of piece attacker

			int color_offset = (victim >> 3) * NTARGETS[atk & 7];

			int index = t_offset + (color_offset + target) * p_offset;

			ATTACK_INDEX[attacker][victim][0] = excl ? -1 : index;
			ATTACK_INDEX[attacker][victim][1] = (excl || semi_excl) ? -1 : index;
		}
	}
}

int threat_index(bool perspective, Square kingsq, Piece attacker, Piece victim, Square src, Square dst) {
	if (perspective) {
		attacker = (Piece)(attacker ^ 8);
		victim = (Piece)(victim ^ 8);
		kingsq = (Square)(kingsq ^ 56);
		src = (Square)(src ^ 56);
		dst = (Square)(dst ^ 56);
	}

	int file = kingsq & 7;
	if (file >= FILE_E) {
		src = (Square)(src ^ 7);
		dst = (Square)(dst ^ 7);
	}

	bool forward = src < dst; // used to do some semi-exclusions

	int index = ATTACK_INDEX[attacker][victim][forward]; // global index of the threat block
	if (index == -1) return -1;

	int offset = OFFSETS[attacker][src]; // second block index within the block
	int piece_index = PIECE_THREAT_INDEX[attacker][src][dst]; // final index

	int res = index + offset + piece_index;

	if (res >= NTHREATS) {
		throw std::runtime_error("trop de menaces");
	}

	return res;
}
