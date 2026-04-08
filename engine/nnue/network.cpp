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
	memcpy(output_weights, ptr, sizeof(output_weights));
	ptr += sizeof(output_weights);
	memcpy(&output_bias, ptr, sizeof(output_bias));
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
	__m256i sum = _mm256_setzero_si256();
	const __m256i zero = _mm256_setzero_si256();
	const __m256i qa_vec = _mm256_set1_epi16(QA);

	for (int i = 0; i < HL_SIZE / 2; i += 16) {
		__m256i stm_vals1 = _mm256_loadu_si256((__m256i *)&stm.val[i]);
		__m256i stm_vals2 = _mm256_loadu_si256((__m256i *)&stm.val[i + HL_SIZE / 2]);
		__m256i ntm_vals1 = _mm256_loadu_si256((__m256i *)&ntm.val[i]);
		__m256i ntm_vals2 = _mm256_loadu_si256((__m256i *)&ntm.val[i + HL_SIZE / 2]);

		stm_vals1 = _mm256_max_epi16(stm_vals1, zero);
		stm_vals1 = _mm256_min_epi16(stm_vals1, qa_vec);
		stm_vals2 = _mm256_max_epi16(stm_vals2, zero);
		stm_vals2 = _mm256_min_epi16(stm_vals2, qa_vec);

		ntm_vals1 = _mm256_max_epi16(ntm_vals1, zero);
		ntm_vals1 = _mm256_min_epi16(ntm_vals1, qa_vec);
		ntm_vals2 = _mm256_max_epi16(ntm_vals2, zero);
		ntm_vals2 = _mm256_min_epi16(ntm_vals2, qa_vec);

		__m256i stm_weights = _mm256_loadu_si256((__m256i *)&net.output_weights[nbucket][i]);
		__m256i ntm_weights = _mm256_loadu_si256((__m256i *)&net.output_weights[nbucket][i + HL_SIZE / 2]);

		__m256i stm_prod = _mm256_mullo_epi16(stm_vals1, stm_weights);
		__m256i ntm_prod = _mm256_mullo_epi16(ntm_vals1, ntm_weights);

		__m256i stm_res = _mm256_madd_epi16(stm_prod, stm_vals2);
		__m256i ntm_res = _mm256_madd_epi16(ntm_prod, ntm_vals2);

		sum = _mm256_add_epi32(sum, stm_res);
		sum = _mm256_add_epi32(sum, ntm_res);
	}

	__m128i sum_128 = _mm_add_epi32(_mm256_extracti128_si256(sum, 0), _mm256_extracti128_si256(sum, 1));
	sum_128 = _mm_add_epi32(sum_128, _mm_shuffle_epi32(sum_128, _MM_SHUFFLE(2, 3, 0, 1)));
	sum_128 = _mm_add_epi32(sum_128, _mm_shuffle_epi32(sum_128, _MM_SHUFFLE(1, 0, 3, 2)));
	int32_t score = _mm_cvtsi128_si32(sum_128);

	score /= QA;
	score += net.output_bias[nbucket];
	score *= SCALE;
	score /= QA * QB;
	return score;
}
