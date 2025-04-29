#include "egnn.hpp"
#include "incbin.h"

extern "C" {
	INCBIN(egnetwork_weights, EGNN_PATH);
}

void EGNetwork::load() {
	char *ptr = (char *)gegnetwork_weightsData;
	memcpy(accumulator_weights, ptr, sizeof(accumulator_weights));
	ptr += sizeof(accumulator_weights);
	memcpy(accumulator_biases, ptr, sizeof(accumulator_biases));
	ptr += sizeof(accumulator_biases);
	memcpy(output_weights, ptr, sizeof(output_weights));
	ptr += sizeof(output_weights);
	memcpy(&output_bias, ptr, sizeof(output_bias));
}

void accumulator_add(const EGNetwork &net, EGAccumulator &acc, uint16_t index) {
	// Note: do not need to manually vectorize this, compiler will do it for us
	for (int i = 0; i < EGHL_SIZE; i++) {
		acc.val[i] += net.accumulator_weights[index][i];
	}
}

void accumulator_sub(const EGNetwork &net, EGAccumulator &acc, uint16_t index) {
	// Note: do not need to manually vectorize this, compiler will do it for us
	for (int i = 0; i < EGHL_SIZE; i++) {
		acc.val[i] -= net.accumulator_weights[index][i];
	}
}

int32_t nnue_eval(const EGNetwork &net, const EGAccumulator &stm, const EGAccumulator &ntm) {
	/// TODO: vectorize
	int32_t score = 0;
	for (int i = 0; i < EGHL_SIZE; i++) {
		int input = std::clamp((int)stm.val[i], 0, QA);
		int weight = input * net.output_weights[i];
		score += input * weight;

		input = std::clamp((int)ntm.val[i], 0, QA);
		weight = input * net.output_weights[EGHL_SIZE + i];
		score += input * weight;
	}
	score /= QA;
	score += net.output_bias;
	score *= SCALE;
	score /= QA * QB;
	return score;
}
