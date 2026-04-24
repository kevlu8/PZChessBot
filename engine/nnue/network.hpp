#pragma once

#include "../includes.hpp"

#define INPUT_SIZE 768
#define NINPUTS 8
#define L1_SIZE 1024
#define L2_SIZE 16
#define L3_SIZE 32
#define NBUCKETS 8
#define SCALE 421
#define QA 255
#define QB 64

constexpr int IBUCKET_LAYOUT[] = {
	0, 2, 4, 6, 7, 5, 3, 1,
	8, 8, 10, 10, 11, 11, 9, 9,
	12, 12, 12, 12, 13, 13, 13, 13,
	12, 12, 12, 12, 13, 13, 13, 13,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
	14, 14, 14, 14, 15, 15, 15, 15,
};

struct Accumulator {
	alignas(32) int16_t val[L1_SIZE] = {};
};

struct alignas(32) Network {
	int16_t accumulator_weights[INPUT_SIZE * NINPUTS][L1_SIZE];
	int16_t accumulator_biases[L1_SIZE];

	int8_t l1_weights[NBUCKETS][L2_SIZE][L1_SIZE];
	float l1_biases[NBUCKETS][L2_SIZE];

	float l2_weights[NBUCKETS][L3_SIZE][L2_SIZE];
	float l2_biases[NBUCKETS][L3_SIZE];

	float output_weights[NBUCKETS][L3_SIZE];
	float output_biases[NBUCKETS];

	void load();
};

int calculate_index(Square sq, PieceType pt, bool side, bool perspective, int nbucket);

int32_t nnue_eval(const Network &net, const Accumulator &stm, const Accumulator &ntm, uint8_t nbucket);

extern Network nnue_network;
