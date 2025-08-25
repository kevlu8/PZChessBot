#include "cutnet.hpp"

float relu(float x) {
	return x > 0 ? x : 0;
}

float sigmoid(float x) {
	return 1.0f / (1.0f + exp(-x));
}

bool CutNet::shouldcut(int depth, int dbeta, bool tentry, int dttDepth, int corrplexity, bool improving, int dmat_eval) {
	float input[] = {depth, dbeta, tentry, dttDepth, corrplexity, improving, dmat_eval};
	// L1
	float hidden[32] = {};
	for (int i = 0; i < 32; i++) {
		hidden[i] = fc1_b[i];
		for (int j = 0; j < 7; j++) {
			hidden[i] += fc1_w[i][j] * input[j];
		}
		hidden[i] = relu(hidden[i]);
	}
	// L2
	float output = fc2_b;
	for (int i = 0; i < 32; i++) {
		output += fc2_w[i] * hidden[i];
	}
	output = sigmoid(output);
	return output > 0.9f;
}