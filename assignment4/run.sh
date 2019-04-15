#!/bin/bash
for N in 10 100 1000
do
	echo Size of N = $N
	for THREADS in 1 2 4 8 16 32
	do
		export OMP_NUM_THREADS=$THREADS
		echo Number of threads = $THREADS >> res_$N.txt
		for i in {1..10}
		do
			aprun -d $THREADS ./bin/nbody.out -n $N >> res_$N.txt
		done
	done 
done
