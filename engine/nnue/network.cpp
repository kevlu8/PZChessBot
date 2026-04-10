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
	alignas(32) int16_t clipped_acc[L0_SIZE * 2];
	alignas(32) float l1[L1_SIZE];
	alignas(32) float l2[L2_SIZE];

	// Preclip the accumulator
	{
		__m256i zero = _mm256_setzero_si256();
		__m256i clip = _mm256_set1_epi16(QA);

		for (int i = 0; i < L0_SIZE; i += 16) {
			__m256i stm_val = _mm256_load_si256((__m256i *)&stm.val[i]);
			__m256i ntm_val = _mm256_load_si256((__m256i *)&ntm.val[i]);

			stm_val = _mm256_max_epi16(stm_val, zero);
			stm_val = _mm256_min_epi16(stm_val, clip);

			ntm_val = _mm256_max_epi16(ntm_val, zero);
			ntm_val = _mm256_min_epi16(ntm_val, clip);

			_mm256_store_si256((__m256i *)&clipped_acc[i], stm_val);
			_mm256_store_si256((__m256i *)&clipped_acc[i + L0_SIZE], ntm_val);
		}
	}

	for (int i = 0; i < L1_SIZE; i++) {
		__m256i sum = _mm256_setzero_si256();

		for (int j = 0; j < L0_SIZE / 2; j += 16) {
			__m256i stm_val1 = _mm256_load_si256((__m256i *)&clipped_acc[j]);
			__m256i stm_val2 = _mm256_load_si256((__m256i *)&clipped_acc[j + L0_SIZE / 2]);
			__m256i ntm_val1 = _mm256_load_si256((__m256i *)&clipped_acc[j + L0_SIZE]);
			__m256i ntm_val2 = _mm256_load_si256((__m256i *)&clipped_acc[j + L0_SIZE + L0_SIZE / 2]);

			__m256i stm_weight = _mm256_cvtepi8_epi16(_mm_load_si128((__m128i *)&net.l1_weights[nbucket][i][j]));
			__m256i ntm_weight = _mm256_cvtepi8_epi16(_mm_load_si128((__m128i *)&net.l1_weights[nbucket][i][j + L0_SIZE / 2]));

			__m256i stm_prod = _mm256_mullo_epi16(stm_val1, stm_weight);
			__m256i ntm_prod = _mm256_mullo_epi16(ntm_val1, ntm_weight);

			__m256i stm_res = _mm256_madd_epi16(stm_val2, stm_prod);
			__m256i ntm_res = _mm256_madd_epi16(ntm_val2, ntm_prod);

			sum = _mm256_add_epi32(sum, stm_res);
			sum = _mm256_add_epi32(sum, ntm_res);
		}

		__m128i sum_128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
		sum_128 = _mm_add_epi32(sum_128, _mm_shuffle_epi32(sum_128, _MM_SHUFFLE(3, 3, 1, 1)));
		sum_128 = _mm_add_epi32(sum_128, _mm_shuffle_epi32(sum_128, _MM_SHUFFLE(3, 2, 3, 2)));

		// l1 is secretly an array of int32
		int32_t f_sum = _mm_cvtsi128_si32(sum_128);
		l1[i] = *(float *)(&f_sum);
	}

	// Convert l1 into a proper float array
	__m256 div = _mm256_set1_ps(1.0f / (QA * QA * QB));
	for (int i = 0; i < L1_SIZE; i += 8) {
		__m256i i_val = _mm256_load_si256((__m256i *)&l1[i]);
		__m256 f_val = _mm256_cvtepi32_ps(i_val);

		__m256 bias = _mm256_load_ps(&net.l1_biases[nbucket][i]);

		f_val = _mm256_fmadd_ps(f_val, div, bias);

		_mm256_store_ps(&l1[i], f_val);
	}

	for (int i = 0; i < L2_SIZE; i++) {
		__m256 sum = _mm256_setzero_ps();
		__m256 zero = _mm256_setzero_ps();
		__m256 clip = _mm256_set1_ps(1.0f);

		for (int j = 0; j < L1_SIZE; j += 8) {
			__m256 val = _mm256_load_ps(&l1[j]);

			val = _mm256_max_ps(val, zero);
			val = _mm256_min_ps(val, clip);

			__m256 weight = _mm256_load_ps(&net.l2_weights[nbucket][i][j]);

			sum = _mm256_fmadd_ps(_mm256_mul_ps(val, val), weight, sum);
		}

		__m128 sum_128 = _mm_add_ps(_mm256_castps256_ps128(sum), _mm256_extractf128_ps(sum, 1));
		sum_128 = _mm_add_ps(sum_128, _mm_movehdup_ps(sum_128));
		sum_128 = _mm_add_ps(sum_128, _mm_movehl_ps(sum_128, sum_128));

		l2[i] = _mm_cvtss_f32(sum_128) + net.l2_biases[nbucket][i];
	}

	float score = net.output_biases[nbucket];
	for (int i = 0; i < L2_SIZE; i++) {
		float val = l2[i];
		val = std::clamp(val, 0.0f, 1.0f);

		float weight = net.output_weights[nbucket][i];

		score += val * val * weight;
	}

	return roundf(score * SCALE);
}
