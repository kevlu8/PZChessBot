#!/bin/bash

ulimit -c unlimited # debug
make clean # Remove previous iteration of network
make -j # Recompile
for i in {0..7}
do
	# Usage: ./pzchessbot [NPOS] [NODES] [OUTFILE]
	seed=$((50000 + i)) # Generate a unique seed for each part
	./pzchessbot 3000000 10000 data.bullet.part$i.txt $seed &
done
wait # Wait for all processes to finish
cat data.bullet.part*.txt > data.bullet.txt # Combine all parts into one file
rm data.bullet.part*.txt # Remove part files to save space
mv data.bullet.txt ../bullet/data.bullet.txt # Move data to trainer directory (since it's large)