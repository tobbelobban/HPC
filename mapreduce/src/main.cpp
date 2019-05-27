#include <iostream>
#include <mpi.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {

	if(argc < 2) {
		return 0;
	}

	MapReduce mp;

	MPI_Init(&argc, &argv);

	mp.init(argv[1]);

	while(mp.remaining_buffer_count > 0) {
		mp.read();
		mp.map();
		mp.reduce();
	}

	MPI_Finalize();

	return 0;
}
