#!/bin/bash

export OMP_NUM_THREADS=32

for i in {1..5}
do
	aprun -n 1 -d $OMP_NUM_THREADS ./stream.out | grep Copy >> 5runs.txt
done
