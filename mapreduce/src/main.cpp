#include <iostream>
#include <mpi.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {

	if(argc < 2) {
		return 0;
	}
	MapReduce mp;
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &mp.world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &mp.world_rank);
	mp.read(argv[1]);
	MPI_Finalize();
	return 0;
}
