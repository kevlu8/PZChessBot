#pragma once

#include "bitboard.hpp"
#include "includes.hpp"

void white_pawn_moves(const Board &board, pzstd::vector<Move> &moves);
void black_pawn_moves(const Board &board, pzstd::vector<Move> &moves);
void pawn_moves(const Board &board, pzstd::vector<Move> &moves);
void knight_moves(const Board &board, pzstd::vector<Move> &moves);
void bishop_moves(const Board &board, pzstd::vector<Move> &moves);
void rook_moves(const Board &board, pzstd::vector<Move> &moves);
void king_moves(const Board &board, pzstd::vector<Move> &moves);
