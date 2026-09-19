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

#pragma once

#include "../bitboard.hpp"
#include "../movegen.hpp"

constexpr int NTARGETS[] = { 3, 5, 4, 4, 5, 0 }; // number of targets for each piece

// Target index [src piece][victim piece]
constexpr int TARGETS[6][6] = {
	{  0,  1, -1,  2, -1, -1 },
	{  0,  1,  2,  3,  4, -1 },
	{  0,  1,  2,  3, -1, -1 },
	{  0,  1,  2,  3, -1, -1 },
	{  0,  1,  2,  3,  4, -1 },
	{ -1, -1, -1, -1, -1, -1 },
};

// [P][src][dst] = ID of the threat by piece P on square src to square dst. don't index it invalidly or we will FIND YOU
extern int PIECE_THREAT_INDEX[14][64][64];

// [P] = total number of attack squares of piece P over all squares (sum of i=0..63 of popcount(calc_attacks(P, i, 0)))
extern int PIECE_OFFSET[14];

// [P] = running offset into the global threat array (60144). accumulated sum of PIECE_OFFSET[0..P-1] * TARGETS[p in 0..P-1] * 2
extern int TOTAL_OFFSET[14];

// [P][sq] = count of attack squares of piece P over all squares 0..sq-1
extern int OFFSETS[14][64];

// [attacker][victim][forwards] = global threat index for the given args
extern int ATTACK_INDEX[14][14][2];

// Returns the global threat index for the given arguments. Returns -1 if there is no threat
int threat_index(bool perspective, Square kingsq, Piece attacker, Piece victim, Square src, Square dst);
