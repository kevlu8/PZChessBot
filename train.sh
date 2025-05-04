#!/bin/bash

ulimit -c unlimited # debug
for it in {3..50}
do
	make clean # Remove previous iteration of network
	make -j # Recompile
	for i in {0..7}
	do
		# Usage: ./pzchessbot [NPOS] [NODES] [OUTFILE]
		seed=$((it * 32 + i)) # Generate a unique seed for each part
		./pzchessbot 3000000 10000 data.bullet.part$i.txt $seed &
	done
	wait # Wait for all processes to finish
	cat data.bullet.part*.txt > data.bullet.txt # Combine all parts into one file
	rm data.bullet.part*.txt # Remove part files to save space
	mv data.bullet.txt ../bullet/data.bullet.txt # Move data to trainer directory (since it's large)
	cd ../bullet/ # Go to trainer directory
	HOST_CXX=g++ CUDA_PATH=/usr/lib/cuda cargo r -r --package bullet-utils convert --from text --input data.bullet.txt --output good.bullet --threads 8 # Convert data to bulletformat
	rm data.bullet.txt # Remove text data to save space
	HOST_CXX=g++ CUDA_PATH=/usr/lib/cuda cargo r -r --example datagen # Train the network
	cp ../PZChessBot/nnue.bin ../PZChessBot/nnue$it.bin # Archive the network
	cp checkpoints/datagen-40/quantised.bin ../PZChessBot/nnue.bin # Copy the trained network back to engine
	cd ../PZChessBot # Go back to engine directory
done