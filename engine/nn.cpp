#include "nn.hpp"

FCLayer::FCLayer(const int inputs, const int outputs) {
	this->inputs = inputs;
	this->outputs = outputs;
	// Initialize the weights to random values
	weights = (float **)malloc(sizeof(float *) * outputs);
	for (int i = 0; i < outputs; i++) {
		weights[i] = (float *)malloc(sizeof(float) * inputs);
		for (int j = 0; j < inputs; j++) {
			weights[i][j] = (float)rand() / (float)RAND_MAX;
		}
	}
}

FCLayer::~FCLayer() {
	for (int i = 0; i < outputs; i++) {
		free(weights[i]);
	}
	free(weights);
}

int FCLayer::load_weights(const char *filename, const int offset) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Error opening file %s.", filename);
		exit(1);
	}
	fseek(fp, offset, SEEK_SET);
	for (int i = 0; i < outputs; i++) {
		fread(weights[i], 4, inputs, fp);
		for (int j = 0; j < inputs; j++) {
			weights[i][j] = __bswap_32(weights[i][j]);
		}
	}
	fclose(fp);
	return offset + outputs * inputs * 4;
}

float *FCLayer::operator()(const float *input) {
	float *output = (float *)malloc(sizeof(float) * outputs);
	for (int i = 0; i < outputs; i++) {
		output[i] = 0;
		for (int j = 0; j < inputs; j++) {
			output[i] += weights[i][j] * input[j];
		}
	}
	return output;
}

RELULayer::RELULayer(const int inputs) { this->inputs = inputs; }

float *RELULayer::operator()(const float *input) {
	float *output = (float *)malloc(sizeof(float) * inputs);
	for (int i = 0; i < inputs; i++) {
		output[i] = input[i] > 0 ? input[i] : 0;
	}
	return output;
}

SigmoidLayer::SigmoidLayer(const int inputs) { this->inputs = inputs; }

float *SigmoidLayer::operator()(const float *input) {
	float *output = (float *)malloc(sizeof(float) * inputs);
	for (int i = 0; i < inputs; i++) {
		output[i] = 1 / (1 + exp(-input[i]));
	}
	return output;
}

NN::NN(const int inputs, const int outputs) {
	this->inputs = inputs;
	this->outputs = outputs;

	// Initialize the layers
	Layer *fc1 = new FCLayer(32, 128);
	Layer *relu1 = new RELULayer(128);
	Layer *fc2 = new FCLayer(128, 64);
	Layer *relu2 = new RELULayer(64);
	Layer *fc3 = new FCLayer(64, 2);
	Layer *sigmoid = new SigmoidLayer(2);
	layers.push_back(fc1);
	layers.push_back(relu1);
	layers.push_back(fc2);
	layers.push_back(relu2);
	layers.push_back(fc3);
	layers.push_back(sigmoid);
}

NN::~NN() {
	for (int i = 0; i < layers.size(); i++) {
		delete layers[i];
	}
}

float *NN::forward(const float *input) {
	float *output = (float *)input;
	float *tmp;
	for (int i = 0; i < layers.size(); i++) {
		tmp = (*layers[i])(output);
		if (i != 0) {
			free(output);
		}
		output = tmp;
	}
	return output;
}
