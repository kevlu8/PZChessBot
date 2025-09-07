#include "cutnet.hpp"

const bool CutNet::shouldcut(const std::array<float, 8>& features) {
	// Layer 1
	float layer1[32];
	for (int i = 0; i < 32; i++) {
		layer1[i] = fc1_bias[i];
		for (int j = 0; j < 8; j++) {
			layer1[i] += fc1_weight[i][j] * features[j];
		}
		// ReLU
		if (layer1[i] < 0) layer1[i] = 0;
	}

	// Layer 2
	float layer2 = fc2_bias;
	for (int i = 0; i < 32; i++) {
		layer2 += fc2_weight[i] * layer1[i];
	}

	// Sigmoid
	float probability = 1.0f / (1.0f + std::exp(-layer2));

	return probability > 0.9f;
}