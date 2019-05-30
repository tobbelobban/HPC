#include <iostream>
#include <mpi.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {

	if(argc < 3) {
		return 0;
	}

	MapReduce mp;

	MPI_Init(&argc, &argv);
		mp.init(argv[1]);
		while(mp.remaining_read > 0) {
			mp.read();
			mp.map();
			mp.reduce();
		}
		mp.write(argv[2]);
		mp.cleanup();
	MPI_Finalize();
	return 0;
}
