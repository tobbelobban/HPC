#include <iostream>
#include <mpi.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {

	if(argc < 2) {
		return 0;
	}

	MapReduce mp;

	MPI_Init(&argc, &argv);
	mp.init();
	mp.read(argv[1]);
	mp.map();

	MPI_Finalize();

	return 0;
}
