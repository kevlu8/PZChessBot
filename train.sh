#!/bin/bash

for it in {3..50}
do
	make clean # Remove previous iteration of network
	make -j # Recompile
	./pzchessbot # Run datagen
	mv data.bullet.txt ../bullet/data.bullet.txt # Move data to trainer directory (since it's large)
	cd ../bullet/ # Go to trainer directory
	HOST_CXX=g++ CUDA_PATH=/usr/lib/cuda cargo r -r --package bullet-utils convert --from text --input data.bullet.txt --output good.bullet --threads 8 # Convert data to bulletformat
	rm data.bullet.txt # Remove text data to save space
	HOST_CXX=g++ CUDA_PATH=/usr/lib/cuda cargo r -r --example datagen # Train the network
	cp ../PZChessBot/nnue.bin ../PZChessBot/nnue$it.bin # Archive the network
	cp checkpoints/datagen-20/quantised.bin ../PZChessBot/nnue.bin # Copy the trained network back to engine
	cd ../PZChessBot # Go back to engine directory
done