#pragma once

#include "bitboard.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "ttable.hpp"
#include "util.hpp"
#include <algorithm>

#define MAX_HISTORY 16384

extern uint64_t nodes;

struct SSEntry {
	Move move;
	Value eval;
};

constexpr Value MVV_LVA_C = 10000;

constexpr Value MVV_LVA[7][7] = { // [victim][attacker]
	// PNBRQKX
	{15, 14, 13, 12, 11, 10, 0}, // Taking a pawn
	{25, 24, 23, 22, 21, 20, 0}, // Taking a knight
	{35, 34, 33, 32, 31, 30, 0}, // Taking a bishop
	{45, 44, 43, 42, 41, 40, 0}, // Taking a rook
	{55, 54, 53, 52, 51, 50, 0}, // Taking a queen
	{0, 0, 0, 0, 0, 0, 0}, // Taking a king (should never happen)
	{0, 0, 0, 0, 0, 0, 0} // No Piece
};

constexpr Value FP_MARGIN = 300;

constexpr Value RFP_MARGIN = 150;

constexpr int ASPIRATION_SIZE = 50;

std::pair<Move, Value> search(Board &board, int64_t time = 1e18, int depth = MAX_PLY, uint64_t nodes = 1e18);

uint64_t perft(Board &board, int depth);
