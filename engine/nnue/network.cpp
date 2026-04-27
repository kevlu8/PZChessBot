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
	const ivec zero = simd::setzero_ivec();
	const ivec clip = simd::broadcast_i16(QA);
	const fvec f_zero = simd::setzero_fvec();
	const fvec f_clip = simd::broadcast_f32(1.0f);
	const fvec div = simd::broadcast_f32(1.0f / (QA * QA * QB / 256.0f));

	alignas(64) int8_t l1[L1_SIZE];
	union {
		alignas(64) int32_t l2i[L2_SIZE];
		alignas(64) float l2[L2_SIZE];
	};
	alignas(64) float l3[L3_SIZE];

	// Pairwise mul
	for (int i = 0; i < L1_SIZE / 2; i += SIMD_LANE_SIZE / 2) {
		ivec stm_val1 = simd::load_ivec((ivec *)&stm.val[i]);
		ivec stm_val2 = simd::load_ivec((ivec *)&stm.val[i + L1_SIZE / 2]);
		ivec ntm_val1 = simd::load_ivec((ivec *)&ntm.val[i]);
		ivec ntm_val2 = simd::load_ivec((ivec *)&ntm.val[i + L1_SIZE / 2]);

		stm_val1 = simd::max_i16(stm_val1, zero);
		stm_val1 = simd::min_i16(stm_val1, clip);
		stm_val2 = simd::max_i16(stm_val2, zero);
		stm_val2 = simd::min_i16(stm_val2, clip);

		ntm_val1 = simd::max_i16(ntm_val1, zero);
		ntm_val1 = simd::min_i16(ntm_val1, clip);
		ntm_val2 = simd::max_i16(ntm_val2, zero);
		ntm_val2 = simd::min_i16(ntm_val2, clip);

		ivec stm_pair = simd::shift_mulhi(stm_val1, stm_val2);
		ivec ntm_pair = simd::shift_mulhi(ntm_val1, ntm_val2);

		simd::store_i16_i8(&l1[i], stm_pair);
		simd::store_i16_i8(&l1[i + L1_SIZE / 2], ntm_pair);
	}

	for (int i = 0; i < L2_SIZE; i += 4) {
		ivec sum0 = zero;
		ivec sum1 = zero;
		ivec sum2 = zero;
		ivec sum3 = zero;

		for (int j = 0; j < L1_SIZE; j += SIMD_LANE_SIZE) {
			ivec val = simd::load_ivec((ivec *)&l1[j]);

			ivec weight0 = simd::load_ivec((ivec *)&net.l1_weights[nbucket][i + 0][j]);
			ivec weight1 = simd::load_ivec((ivec *)&net.l1_weights[nbucket][i + 1][j]);
			ivec weight2 = simd::load_ivec((ivec *)&net.l1_weights[nbucket][i + 2][j]);
			ivec weight3 = simd::load_ivec((ivec *)&net.l1_weights[nbucket][i + 3][j]);

			sum0 = simd::accdp_u8i8_i16(val, weight0, sum0);
			sum1 = simd::accdp_u8i8_i16(val, weight1, sum1);
			sum2 = simd::accdp_u8i8_i16(val, weight2, sum2);
			sum3 = simd::accdp_u8i8_i16(val, weight3, sum3);
		}

		l2i[i + 0] = simd::reduce_add_epi16(sum0);
		l2i[i + 1] = simd::reduce_add_epi16(sum1);
		l2i[i + 2] = simd::reduce_add_epi16(sum2);
		l2i[i + 3] = simd::reduce_add_epi16(sum3);
	}

	// Convert l2 into a proper float array
	for (int i = 0; i < L2_SIZE; i += SIMD_LANE_SIZE / 4) {
		ivec i_val = simd::load_ivec((ivec *)&l2i[i]);
		fvec val = simd::cvt_i32_f32(i_val);

		fvec bias = simd::load_fvec(&net.l1_biases[nbucket][i]);

		val = simd::fma_f32(val, div, bias);

		val = simd::max_f32(val, f_zero);
		val = simd::min_f32(val, f_clip);

		val = simd::mul_f32(val, val);

		simd::store_f32(&l2[i], val);
	}

#if defined(__AVX512BW__)
	for (int i = 0; i < L3_SIZE; i += 8 * 2) {
		fvec sum0 = simd::load_fvec(&net.l2_biases[nbucket][i + 0]);
		fvec sum1 = simd::load_fvec(&net.l2_biases[nbucket][i + 8]);

		for (int j = 0; j < L2_SIZE; j++) {
			fvec val = simd::broadcast_f32(l2[j]);

			fvec weight0 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 0]);
			fvec weight1 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 8]);

			sum0 = simd::fma_f32(val, weight0, sum0);
			sum1 = simd::fma_f32(val, weight1, sum1);
		}

		simd::store_f32(&l3[i + 0], sum0);
		simd::store_f32(&l3[i + 8], sum1);
	}
#else
	for (int i = 0; i < L3_SIZE; i += 8 * 4) {
		fvec sum0 = simd::load_fvec(&net.l2_biases[nbucket][i + 0x00]);
		fvec sum1 = simd::load_fvec(&net.l2_biases[nbucket][i + 0x08]);
		fvec sum2 = simd::load_fvec(&net.l2_biases[nbucket][i + 0x10]);
		fvec sum3 = simd::load_fvec(&net.l2_biases[nbucket][i + 0x18]);

		for (int j = 0; j < L2_SIZE; j++) {
			fvec val = simd::broadcast_f32(l2[j]);

			fvec weight0 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 0x00]);
			fvec weight1 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 0x08]);
			fvec weight2 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 0x10]);
			fvec weight3 = simd::load_fvec(&net.l2_weights[nbucket][j][i + 0x18]);

			sum0 = simd::fma_f32(val, weight0, sum0);
			sum1 = simd::fma_f32(val, weight1, sum1);
			sum2 = simd::fma_f32(val, weight2, sum2);
			sum3 = simd::fma_f32(val, weight3, sum3);
		}

		simd::store_f32(&l3[i + 0x00], sum0);
		simd::store_f32(&l3[i + 0x08], sum1);
		simd::store_f32(&l3[i + 0x10], sum2);
		simd::store_f32(&l3[i + 0x18], sum3);
	}
#endif

	fvec sum = f_zero;
	for (int i = 0; i < L3_SIZE; i += SIMD_LANE_SIZE / 4) {
		fvec val = simd::load_fvec(&l3[i]);

		val = simd::max_f32(val, f_zero);
		val = simd::min_f32(val, f_clip);

		fvec weight = simd::load_fvec(&net.output_weights[nbucket][i]);

		sum = simd::fma_f32(simd::mul_f32(val, val), weight, sum);
	}

	float score = simd::reduce_add_ps(sum) + net.output_biases[nbucket];

	return score * SCALE;
}
