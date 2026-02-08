#pragma once

#include "../includes.hpp"

#define INPUT_SIZE 768
#define NINPUTS 4
#define HL_SIZE 1024
#define NBUCKETS 8
#define SCALE 338
#define QA 255
#define QB 64

struct Accumulator {
	int16_t val[HL_SIZE] = {};
};

struct Network {
	int16_t accumulator_weights[INPUT_SIZE * NINPUTS][HL_SIZE];
	int16_t accumulator_biases[HL_SIZE];
	int16_t output_weights[NBUCKETS][2 * HL_SIZE];
	int16_t output_bias[NBUCKETS];

	void load();
};

int calculate_index(Square sq, PieceType pt, bool side, bool perspective, int nbucket);

void accumulator_add(const Network &net, Accumulator &acc, uint16_t index);

void accumulator_sub(const Network &net, Accumulator &acc, uint16_t index);

int32_t nnue_eval(const Network &net, const Accumulator &stm, const Accumulator &ntm, uint8_t nbucket);

extern Network nnue_network;
