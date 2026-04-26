#include "network.hpp"
#include "simd.hpp"

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

	for (int i = 0; i < NBUCKETS; i++) {
		for (int j = 0; j < L3_SIZE; j++) {
			for (int k = 0; k < L2_SIZE; k++) {
				memcpy(&l2_weights[i][k][j], ptr, 4);
				ptr += 4;
			}
		}
	}

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
	const __m256i zero = _mm256_setzero_si256();
	const __m256i clip = _mm256_set1_epi16(QA);
	const __m256 f_zero = _mm256_setzero_ps();
	const __m256 f_clip = _mm256_set1_ps(1.0f);
	const __m256 div = _mm256_set1_ps(1.0f / (QA * QA * QB / 256.0f));

	alignas(32) int8_t l1[L1_SIZE];
	union {
		alignas(32) int32_t l2i[L2_SIZE];
		alignas(32) float l2[L2_SIZE];
	};
	alignas(32) float l3[L3_SIZE];

	// Pairwise mul
	for (int i = 0; i < L1_SIZE / 2; i += 16) {
		__m256i stm_val1 = _mm256_load_si256((__m256i *)&stm.val[i]);
		__m256i stm_val2 = _mm256_load_si256((__m256i *)&stm.val[i + L1_SIZE / 2]);
		__m256i ntm_val1 = _mm256_load_si256((__m256i *)&ntm.val[i]);
		__m256i ntm_val2 = _mm256_load_si256((__m256i *)&ntm.val[i + L1_SIZE / 2]);

		stm_val1 = _mm256_max_epi16(stm_val1, zero);
		stm_val1 = _mm256_min_epi16(stm_val1, clip);
		stm_val2 = _mm256_max_epi16(stm_val2, zero);
		stm_val2 = _mm256_min_epi16(stm_val2, clip);

		ntm_val1 = _mm256_max_epi16(ntm_val1, zero);
		ntm_val1 = _mm256_min_epi16(ntm_val1, clip);
		ntm_val2 = _mm256_max_epi16(ntm_val2, zero);
		ntm_val2 = _mm256_min_epi16(ntm_val2, clip);

		stm_val1 = _mm256_slli_epi16(stm_val1, 7);
		ntm_val1 = _mm256_slli_epi16(ntm_val1, 7);

		__m256i stm_pair = _mm256_mulhrs_epi16(stm_val1, stm_val2);
		__m256i ntm_pair = _mm256_mulhrs_epi16(ntm_val1, ntm_val2);

		simd::store_epi16_epi8(&l1[i], stm_pair);
		simd::store_epi16_epi8(&l1[i + L1_SIZE / 2], ntm_pair);
	}

	for (int i = 0; i < L2_SIZE; i += 4) {
		__m256i sum0 = _mm256_setzero_si256();
		__m256i sum1 = _mm256_setzero_si256();
		__m256i sum2 = _mm256_setzero_si256();
		__m256i sum3 = _mm256_setzero_si256();

		for (int j = 0; j < L1_SIZE; j += 32) {
			__m256i val = _mm256_load_si256((__m256i *)&l1[j]);

			__m256i weight0 = _mm256_load_si256((__m256i *)&net.l1_weights[nbucket][i + 0][j]);
			__m256i weight1 = _mm256_load_si256((__m256i *)&net.l1_weights[nbucket][i + 1][j]);
			__m256i weight2 = _mm256_load_si256((__m256i *)&net.l1_weights[nbucket][i + 2][j]);
			__m256i weight3 = _mm256_load_si256((__m256i *)&net.l1_weights[nbucket][i + 3][j]);

			__m256i res0 = _mm256_maddubs_epi16(val, weight0);
			__m256i res1 = _mm256_maddubs_epi16(val, weight1);
			__m256i res2 = _mm256_maddubs_epi16(val, weight2);
			__m256i res3 = _mm256_maddubs_epi16(val, weight3);

			sum0 = _mm256_add_epi16(res0, sum0);
			sum1 = _mm256_add_epi16(res1, sum1);
			sum2 = _mm256_add_epi16(res2, sum2);
			sum3 = _mm256_add_epi16(res3, sum3);
		}

		l2i[i + 0] = simd::reduce_add_epi16(sum0);
		l2i[i + 1] = simd::reduce_add_epi16(sum1);
		l2i[i + 2] = simd::reduce_add_epi16(sum2);
		l2i[i + 3] = simd::reduce_add_epi16(sum3);
	}

	// Convert l2 into a proper float array
	for (int i = 0; i < L2_SIZE; i += 8) {
		__m256i i_val = _mm256_load_si256((__m256i *)&l2i[i]);
		__m256 val = _mm256_cvtepi32_ps(i_val);

		__m256 bias = _mm256_load_ps(&net.l1_biases[nbucket][i]);

		val = _mm256_fmadd_ps(val, div, bias);

		val = _mm256_max_ps(val, f_zero);
		val = _mm256_min_ps(val, f_clip);

		val = _mm256_mul_ps(val, val);

		_mm256_store_ps(&l2[i], val);
	}

	for (int i = 0; i < L3_SIZE; i += 8 * 4) {
		__m256 sum0 = _mm256_load_ps(&net.l2_biases[nbucket][i + 0x00]);
		__m256 sum1 = _mm256_load_ps(&net.l2_biases[nbucket][i + 0x08]);
		__m256 sum2 = _mm256_load_ps(&net.l2_biases[nbucket][i + 0x10]);
		__m256 sum3 = _mm256_load_ps(&net.l2_biases[nbucket][i + 0x18]);

		for (int j = 0; j < L2_SIZE; j++) {
			__m256 val = _mm256_set1_ps(l2[j]);

			__m256 weight0 = _mm256_load_ps(&net.l2_weights[nbucket][j][i + 0x00]);
			__m256 weight1 = _mm256_load_ps(&net.l2_weights[nbucket][j][i + 0x08]);
			__m256 weight2 = _mm256_load_ps(&net.l2_weights[nbucket][j][i + 0x10]);
			__m256 weight3 = _mm256_load_ps(&net.l2_weights[nbucket][j][i + 0x18]);

			sum0 = _mm256_fmadd_ps(val, weight0, sum0);
			sum1 = _mm256_fmadd_ps(val, weight1, sum1);
			sum2 = _mm256_fmadd_ps(val, weight2, sum2);
			sum3 = _mm256_fmadd_ps(val, weight3, sum3);
		}

		_mm256_store_ps(&l3[i + 0x00], sum0);
		_mm256_store_ps(&l3[i + 0x08], sum1);
		_mm256_store_ps(&l3[i + 0x10], sum2);
		_mm256_store_ps(&l3[i + 0x18], sum3);
	}

	__m256 sum = _mm256_setzero_ps();
	for (int i = 0; i < L3_SIZE; i += 8) {
		__m256 val = _mm256_load_ps(&l3[i]);

		val = _mm256_max_ps(val, f_zero);
		val = _mm256_min_ps(val, f_clip);

		__m256 weight = _mm256_load_ps(&net.output_weights[nbucket][i]);

		sum = _mm256_fmadd_ps(_mm256_mul_ps(val, val), weight, sum);
	}

	__m128 sum_128 = _mm_add_ps(_mm256_castps256_ps128(sum), _mm256_extractf128_ps(sum, 1));
	sum_128 = _mm_add_ps(sum_128, _mm_movehdup_ps(sum_128));
	sum_128 = _mm_add_ps(sum_128, _mm_movehl_ps(sum_128, sum_128));
	float score = _mm_cvtss_f32(sum_128) + net.output_biases[nbucket];

	return score * SCALE;
}
