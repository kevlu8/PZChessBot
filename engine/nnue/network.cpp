#include "network.hpp"
#include "incbin.h"

extern "C" {
	INCBIN(network_weights, NNUE_PATH);
}

void Network::load() {
	char *ptr = (char *)gnetwork_weightsData;

	memcpy(accumulator_weights, ptr, sizeof(accumulator_weights));
	ptr += sizeof(accumulator_weights);

	memcpy(accumulator_biases, ptr, sizeof(accumulator_biases));
	ptr += sizeof(accumulator_biases);

	memcpy(l1_weights, ptr, sizeof(l1_weights));
	ptr += sizeof(l1_weights);

	memcpy(l1_biases, ptr, sizeof(l1_biases));
	ptr += sizeof(l1_biases);

	memcpy(l2_weights, ptr, sizeof(l2_weights));
	ptr += sizeof(l2_weights);

	memcpy(l2_biases, ptr, sizeof(l2_biases));
	ptr += sizeof(l2_biases);

	memcpy(output_weights, ptr, sizeof(output_weights));
	ptr += sizeof(output_weights);

	memcpy(&output_biases, ptr, sizeof(output_biases));
}

int calculate_index(Square sq, PieceType pt, bool side, bool perspective, int nbucket) {
	if (nbucket & 1) {
		// Flip the square
		sq = (Square)(sq ^ 7);
	}
	nbucket /= 2;
	if (perspective) {
		side = !side;
		sq = (Square)(sq ^ 56);
	}
	return nbucket * INPUT_SIZE + side * 64 * 6 + pt * 64 + sq;
}

int32_t nnue_eval(const Network &net, const Accumulator &stm, const Accumulator &ntm, uint8_t nbucket) {
	float l1[L1_SIZE];
	float l2[L2_SIZE];

	for (int i = 0; i < L1_SIZE; i++) {
		int32_t sum = 0;

		for (int j = 0; j < L0_SIZE / 2; j++) {
			int16_t stm_val1 = stm.val[j];
			int16_t stm_val2 = stm.val[j + L0_SIZE / 2];
			int16_t ntm_val1 = ntm.val[j];
			int16_t ntm_val2 = ntm.val[j + L0_SIZE / 2];

			stm_val1 = std::clamp(stm_val1, (int16_t)0, (int16_t)QA);
			stm_val2 = std::clamp(stm_val2, (int16_t)0, (int16_t)QA);
			ntm_val1 = std::clamp(ntm_val1, (int16_t)0, (int16_t)QA);
			ntm_val2 = std::clamp(ntm_val2, (int16_t)0, (int16_t)QA);

			int32_t stm_weight = net.l1_weights[nbucket][i][j];
			int32_t ntm_weight = net.l1_weights[nbucket][i][j + L0_SIZE / 2];

			sum += stm_weight * stm_val1 * stm_val2;
			sum += ntm_weight * ntm_val1 * ntm_val2;
		}

		float f_sum = sum;
		f_sum /= QA * QA * QB;
		f_sum += net.l1_biases[nbucket][i];

		l1[i] = f_sum;
	}

	for (int i = 0; i < L2_SIZE; i++) {
		float sum = net.l2_biases[nbucket][i];

		for (int j = 0; j < L1_SIZE; j++) {
			float val = l1[j];
			val = std::clamp(val, 0.0f, 1.0f);

			float weight = net.l2_weights[nbucket][i][j];

			sum += val * val * weight;
		}

		l2[i] = sum;
	}

	float sum = net.output_biases[nbucket];
	for (int i = 0; i < L2_SIZE; i++) {
		float val = l2[i];
		val = std::clamp(val, 0.0f, 1.0f);

		float weight = net.output_weights[nbucket][i];

		sum += val * val * weight;
	}

	return roundf(sum * SCALE);
}
