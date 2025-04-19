#pragma once

#include "../includes.hpp"

#define INPUT_SIZE 768
#define HL_SIZE 128
#define SCALE 400
#define QA 255
#define QB 64

struct Accumulator {
	int16_t val[HL_SIZE] = {};
};

struct Network {
	int16_t accumulator_weights[INPUT_SIZE][HL_SIZE];
	int16_t accumulator_biases[HL_SIZE];
	int16_t output_weights[2 * HL_SIZE];
	int16_t output_bias;

	void load();
};

int calculate_index(Square sq, PieceType pt, bool side, bool perspective);

void accumulator_add(const Network &net, Accumulator &acc, uint16_t index);

void accumulator_sub(const Network &net, Accumulator &acc, uint16_t index);

int32_t nnue_eval(const Network &net, const Accumulator &stm, const Accumulator &ntm);