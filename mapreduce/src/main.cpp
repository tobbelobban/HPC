#include <unistd.h>
#include <iostream>
#include <mpi.h>
#include <math.h>
#include "mapreduce.hpp"


void update_times( const double * start_time, const double * end_time, double * avg_time, double * prev_avg_time, double *
stddev_time, const int iteration,
const int repeat ) {
	*prev_avg_time = *avg_time;
	*avg_time = *avg_time + ( (*end_time - *start_time) - *avg_time ) / (iteration+1);
	*stddev_time = *stddev_time + ( (*end_time - *start_time) - *avg_time ) * ( (*end_time - *start_time) -
*prev_avg_time);
}

int main(int argc, char ** argv) {
	int world_rank, world_size;
	int repeat = 1;
	int opt;
	MapReduce mp;

	// time keepers
	double avg_runtime = 0.0, avg_read_map_time = 0.0, avg_reduce_time = 0.0, avg_write_time = 0.0;
	double prev_avg_runtime = 0.0, prev_avg_read_map_time = 0.0, prev_avg_reduce_time = 0.0, prev_avg_write_time = 0.0;
	double stddev_runtime = 0.0, stddev_read_map_time = 0.0, stddev_reduce_time = 0.0, stddev_write_time = 0.0;
	double start_time, end_time, tmp_start_time, tmp_end_time;


	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );

	while( (opt = getopt(argc, argv, "w:r:")) != -1 ) {
		switch ( opt ) {
			case 'r':
					repeat = atoi(optarg);
					if( repeat < 1 ) {
						if( world_rank == MASTER )
							std::cout << "Please supply positive repeat count with flag -r." <<
std::endl;
						MPI_Finalize();
						exit(1);
					}
					break;
			case 'w':
					mp.wordlen = atoi(optarg);
					if( mp.wordlen < 1 ) {
						if( world_rank == MASTER )
							std::cout << "Please supply positive word length with flag -w" <<
std::endl;
						MPI_Finalize();
						exit(1);
					}
					break;
			default:
					if( world_rank == MASTER ) {
						std::cout << "Unknown flag: " << opt << std::endl;
					}
					MPI_Finalize();
					exit(1);
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
		tmp_start_time = MPI_Wtime();
		while( mp.remaining_read > 0 ) {
			mp.read();
			mp.map();
		}
		tmp_end_time = MPI_Wtime();

		//update read & map times
		update_times( &tmp_start_time, &tmp_end_time, &avg_read_map_time, &prev_avg_read_map_time,
&stddev_read_map_time, iteration, repeat );

		// finally we reduce
		tmp_start_time = MPI_Wtime();
		mp.reduce();
		tmp_end_time = MPI_Wtime();

		// update reduce times
		update_times( &tmp_start_time, &tmp_end_time, &avg_reduce_time, &prev_avg_reduce_time, &stddev_reduce_time,
iteration, repeat );

		// then write
		tmp_start_time = MPI_Wtime();
		mp.write(argv[optind+1]);
		tmp_end_time = MPI_Wtime();

		// update write times
		update_times( &tmp_start_time, &tmp_end_time, &avg_write_time, &prev_avg_write_time, &stddev_write_time,
iteration, repeat );

		// cleanup
		mp.cleanup();

		// don't get end_time until the last process has finished
		MPI_Barrier( MPI_COMM_WORLD );
		end_time = MPI_Wtime();
		// update total time
		update_times( &start_time, &end_time, &avg_runtime, &prev_avg_runtime, &stddev_runtime, iteration, repeat );

		if( world_rank == MASTER)
			std::cout << "run: " << iteration << "\t\t= " << end_time - start_time << 's' << std::endl;
	}

	if( world_rank == MASTER ) {
		stddev_runtime = sqrt(stddev_runtime / (repeat - 1));
		stddev_read_map_time = sqrt(stddev_read_map_time / (repeat - 1));
		stddev_reduce_time = sqrt(stddev_reduce_time / (repeat - 1));
		stddev_write_time = sqrt(stddev_write_time / (repeat - 1));

		std::cout << "avg. duration\t= " << avg_runtime << " ± " << stddev_runtime << std::endl;
		std::cout << "avg. read&map\t= " << avg_read_map_time << " ± " << stddev_read_map_time << std::endl;
		std::cout << "avg. reduce\t= " << avg_reduce_time << " ± " << stddev_reduce_time << std::endl;
		std::cout << "avg. write\t= " << avg_write_time << " ± " << stddev_write_time << std::endl;
		std::cout << std::endl;
	}
	// à la fin
	MPI_Finalize();

	return 0;
}
