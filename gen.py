import numpy as np

# Replace these with your actual dimensions
ACCUMULATOR_WEIGHTS_SHAPE = (32, 768)
ACCUMULATOR_BIASES_SHAPE = (32,)
OUTPUT_WEIGHTS_SHAPE = (1, 32,)
OUTPUT_BIAS_SHAPE = (1,)

# Random value ranges
ACCUMULATOR_RANGE = (-128, 128)
OUTPUT_RANGE = (-128, 128)

# Generate weights and biases
accumulator_weights = np.random.randint(*ACCUMULATOR_RANGE, size=ACCUMULATOR_WEIGHTS_SHAPE, dtype=np.int16)
accumulator_biases = np.random.randint(*ACCUMULATOR_RANGE, size=ACCUMULATOR_BIASES_SHAPE, dtype=np.int16)
output_weights = np.random.randint(*OUTPUT_RANGE, size=OUTPUT_WEIGHTS_SHAPE, dtype=np.int16)
output_bias = np.random.randint(*OUTPUT_RANGE, size=OUTPUT_BIAS_SHAPE, dtype=np.int16)

# Write to binary file in correct order
with open("nnue.bin", "wb") as f:
    f.write(accumulator_weights.tobytes())
    f.write(accumulator_biases.tobytes())
    f.write(output_weights.tobytes())
    f.write(output_bias.tobytes())
