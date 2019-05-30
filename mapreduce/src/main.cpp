#include <iostream>
#include <mpi.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {
	int world_rank, world_size;
	MapReduce mp;

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );
	if( argc < 3 ) {
		if(world_rank == MASTER) {
			std::cout << "Usage: mpirun ./path-to-binary input.txt output.txt" << std::endl;
		}
		MPI_Finalize();
		exit(1);
	}

	mp.init(argv[1]);

	// continue reading -> mapping until out of data
	while(mp.remaining_read > 0) {
		mp.read();
		mp.map();
	}
	// finally we reduce
	mp.reduce();

	// then write
	mp.write(argv[2]);

	// cleanup
	mp.cleanup();

	// à la fin 
	MPI_Finalize();

	return 0;
}
