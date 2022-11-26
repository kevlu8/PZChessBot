#include <cmath>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

// how does this work what do i put in it
class Layer {
public:
	virtual float *operator()(const float *) = 0;
};

class FCLayer : public Layer {
private:
	float **weights;
	int inputs, outputs;

public:
	FCLayer(const int, const int);
	~FCLayer();
	int load_weights(const char *, const int);
	float *operator()(const float *);
};

class RELULayer : public Layer {
private:
	int inputs;

public:
	RELULayer(const int);
	float *operator()(const float *);
};

class SigmoidLayer : public Layer {
private:
	int inputs;

public:
	SigmoidLayer(const int);
	float *operator()(const float *);
};

class SoftmaxLayer : public Layer {
private:
	int inputs;

public:
	SoftmaxLayer(const int);
	float *operator()(const float *);
};

class NN {
private:
	int inputs, outputs;
	std::vector<Layer *> layers;

public:
	NN(const int, const int);
	~NN();
	float *forward(const float *);
	void load_weights(const char *);
};
