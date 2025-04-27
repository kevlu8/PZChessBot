#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string>
#include <utility>

#include "pzstl/vector.hpp"

#define VERSION "v20250427T15"

typedef uint64_t Bitboard;

constexpr bool WHITE = false;
constexpr bool BLACK = true;

constexpr int MAX_PLY = 100;

typedef int16_t Value;
constexpr Value VALUE_ZERO = 0;
constexpr Value VALUE_INFINITE = 32000;
constexpr Value VALUE_MATE = 30002; // Add 2 as a consequence of our evaluation function only returning MATE when king is taken
constexpr Value VALUE_MATE_MAX_PLY = VALUE_MATE - MAX_PLY;

constexpr Value PawnValue = 100;
constexpr Value KnightValue = 350;
constexpr Value BishopValue = 350;
constexpr Value RookValue = 525;
constexpr Value QueenValue = 1000;
constexpr Value VALUE_MAX = QueenValue * 9 + (KnightValue + BishopValue + RookValue) * 2;

#ifdef HCE
#define CP_SCALE_FACTOR 4
#else
#define CP_SCALE_FACTOR 1
#endif

#ifndef NNUE_PATH
#define NNUE_PATH "nnue.bin"
#endif

#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)

enum PieceType : uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECETYPE };

enum Piece : uint8_t {
	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN = 8,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
	NO_PIECE
};

constexpr Value PieceValue[] = {PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_INFINITE - VALUE_MAX};

enum CastlingRights : uint8_t { NO_CASTLE, WHITE_OO, WHITE_OOO = WHITE_OO << 1, BLACK_OO = WHITE_OO << 2, BLACK_OOO = WHITE_OO << 3 };

// clang-format off
constexpr PieceType letter_piece[] = {BISHOP, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, KING, NO_PIECETYPE, NO_PIECETYPE, KNIGHT, NO_PIECETYPE, PAWN, QUEEN, ROOK};
constexpr char piecetype_letter[] = {'p', 'n', 'b', 'r', 'q', 'k', '?'};
constexpr char piece_letter[] = {'P','N','B','R','Q','K','?','?','p','n','b','r','q','k','?'};
// clang-format on

enum File : uint16_t {
	FILE_A,
	FILE_B,
	FILE_C,
	FILE_D,
	FILE_E,
	FILE_F,
	FILE_G,
	FILE_H,
};

enum Rank : uint16_t {
	RANK_1,
	RANK_2,
	RANK_3,
	RANK_4,
	RANK_5,
	RANK_6,
	RANK_7,
	RANK_8,
};

// clang-format off
enum Square : uint16_t {
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_NONE
};
// clang-format on

enum MoveType {
	NORMAL,
	PROMOTION = 1 << 14,
	EN_PASSANT = 2 << 14,
	CASTLING = 3 << 14,
};
