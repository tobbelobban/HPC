#!/bin/bash

#SBATCH -J omp_schedule
#SBATCH -t 00:15:00
#SBATCH -A edu19.DD2356
#SBATCH --nodes=1

for j in {1..10}
do
	export OMP_NUM_THREADS=32
	aprun -n 1 -d $OMP_NUM_THREADS ./stream_dynamic.out | grep Copy >> dynamic.txt
	aprun -n 1 -d $OMP_NUM_THREADS ./stream_static.out | grep Copy >> static.txt
	aprun -n 1 -d $OMP_NUM_THREADS ./stream_guided.out | grep Copy >> guided.txt
done
