#!/bin/bash

for i in 1 2 4 8 16 32
do
	export OMP_NUM_THREADS=$i
	echo "Function    Best Rate MB/s  Avg time     Min time     Max time" > threads_$i.txt
	for j in {1..10}
	do
		aprun -n 1 -d $OMP_NUM_THREADS ./stream.out | grep Copy >> threads_$i.txt
	done
done
