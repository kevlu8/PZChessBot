#pragma once

#include "../includes.hpp"
#include "network.hpp" // For definitions like INPUT_SIZE, etc

#define EGHL_SIZE 256

struct EGAccumulator {
	int16_t val[EGHL_SIZE] = {};
};

struct EGNetwork {
	int16_t accumulator_weights[INPUT_SIZE][EGHL_SIZE];
	int16_t accumulator_biases[EGHL_SIZE];
	int16_t output_weights[2 * EGHL_SIZE];
	int16_t output_bias;

	void load();
};

int calculate_index(Square sq, PieceType pt, bool side, bool perspective);

void accumulator_add(const EGNetwork &net, EGAccumulator &acc, uint16_t index);

void accumulator_sub(const EGNetwork &net, EGAccumulator &acc, uint16_t index);

int32_t nnue_eval(const EGNetwork &net, const EGAccumulator &stm, const EGAccumulator &ntm);