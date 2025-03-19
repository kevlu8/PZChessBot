#include "network.hpp"

int16_t crelu(int16_t x) {
	if (x < 0) return 0;
	if (x > QA) return QA;
	return x;
}

void Network::load(const std::string &filename) {
	std::ifstream file(filename, std::ios::binary);
	// bin format: little endian, 2 bytes per int16_t
	if (!file) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}
	file.read(reinterpret_cast<char *>(accumulator_weights), sizeof(accumulator_weights));
	file.read(reinterpret_cast<char *>(accumulator_biases), sizeof(accumulator_biases));
	file.read(reinterpret_cast<char *>(output_weights), sizeof(output_weights));
	file.read(reinterpret_cast<char *>(&output_bias), sizeof(output_bias));
	file.close();
}

int calculate_index(Square sq, PieceType pt, bool side, bool perspective) {
	if (perspective) {
		side = !side;
		sq = (Square)(sq ^ 56);
	}
	return side * 64 * 6 + pt * 64 + sq;
}

void accumulator_add(const Network &net, Accumulator &acc, uint16_t index) {
	for (int i = 0; i < HL_SIZE; i++) {
		acc.val[i] += net.accumulator_weights[index][i];
	}
}

void accumulator_sub(const Network &net, Accumulator &acc, uint16_t index) {
	for (int i = 0; i < HL_SIZE; i++) {
		acc.val[i] -= net.accumulator_weights[index][i];
	}
}

int32_t nnue_eval(const Network &net, const Accumulator &stm, const Accumulator &ntm) {
	int32_t score = 0;
	for (int i = 0; i < HL_SIZE; i++) {
		score += crelu(stm.val[i]) * net.output_weights[i];
		score += crelu(ntm.val[i]) * net.output_weights[HL_SIZE + i];
	}
	score += net.output_bias;
	score *= SCALE;
	score /= QA * QB;
	return score;
}