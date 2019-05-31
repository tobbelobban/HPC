#include <unistd.h>
#include <iostream>
#include <mpi.h>
#include <math.h>
#include "mapreduce.hpp"

int main(int argc, char ** argv) {
	int world_rank, world_size;
	int repeat = 1;
	int opt;
	MapReduce mp;

	double avg_runtime = 0.0, prev_avg_runtime = 0.0, stddev_runtime = 0.0;
	double start_time, end_time;

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );

	while( (opt = getopt(argc, argv, "r:")) != -1 ) {
		switch ( opt ) {
			case 'r':
					repeat = atoi(optarg);
					if( repeat < 1 ) {
						if( world_rank == MASTER )
							std::cout << "Please supply positive repeat count." << std::endl;
						MPI_Finalize();
						exit(1);
					}
					break;
			default:
					break;
		}
	}

	if( argv[optind] == NULL || argv[optind + 1] == NULL ) {
		if(world_rank == MASTER) {
			std::cout << "Usage: " << argv[0] << " [flags] [input file] [output file]" << std::endl;
		}
		MPI_Finalize();
		exit(1);
	}

	// map reduce 'repeat' times
	for(int iteration = 0; iteration < repeat; ++iteration) {
		MPI_Barrier( MPI_COMM_WORLD );
		start_time = MPI_Wtime();

		mp.init(argv[optind]);

		// continue reading -> mapping until out of data
		while( mp.remaining_read > 0 ) {
			mp.read();
			mp.map();
		}
		// finally we reduce
		mp.reduce();

		// then write
		mp.write(argv[optind+1]);

		// cleanup
		mp.cleanup();

		// don't get end_time until the last process has finished
		MPI_Barrier( MPI_COMM_WORLD );
		end_time = MPI_Wtime();

		if( world_rank == MASTER ) {
			std::cout << "run: " << iteration << "\t\t= " << end_time - start_time << 's' << std::endl;
		}
		prev_avg_runtime = avg_runtime;
		avg_runtime = avg_runtime + ( (end_time - start_time) - avg_runtime ) / (iteration+1);
		stddev_runtime = stddev_runtime + ( (end_time - start_time) - avg_runtime ) * ( (end_time - start_time) - prev_avg_runtime);

		if( world_rank == MASTER ) {
			stddev_runtime = std::sqrt(stddev_runtime / (repeat - 1));
			std::cout << "duration\t= " << avg_runtime << " ± " << stddev_runtime << std::endl;
		}
	}

	// à la fin
	MPI_Finalize();

	return 0;
}
