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

#include "includes.hpp"

constexpr double as[] = {-30.98361896, 78.24679219, -88.99642086, 138.22393411};
constexpr double bs[] = {2.60484417, 11.76498263, -15.13585141, 26.19189708};

std::tuple<int, int, int> score_to_wdl(Position &pos, Value score) {
	if (score >= VALUE_WIN)
		return {1000, 0, 0};
	if (score <= -VALUE_WIN)
		return {0, 0, 1000};

	int mat = 0;
	for (int i = 0; i < 64; i++) {
		PieceType pt = PieceType(pos.mailbox[i] & 7);
		if (pt == NO_PIECETYPE) continue;

		switch (pt) {
			case PAWN:
				mat += 1;
				break;
			case KNIGHT:
				mat += 3;
				break;
			case BISHOP:
				mat += 3;
				break;
			case ROOK:
				mat += 5;
				break;
			case QUEEN:
				mat += 9;
				break;
		}
	}

	double x = score / 2.75;
	double p_a = ((as[0] * mat / 58.0 + as[1]) * mat / 58.0 + as[2]) * mat / 58.0 + as[3];
	double p_b = ((bs[0] * mat / 58.0 + bs[1]) * mat / 58.0 + bs[2]) * mat / 58.0 + bs[3];
	double p = 1.0 / (1.0 + exp(-(x - p_a) / p_b));
	int w = int(round(p * 1000));
	p = 1.0 / (1.0 + exp(-(-x - p_a) / p_b));
	int l = int(round(p * 1000));
	int d = 1000 - w - l;
	return {w, d, l};
}
