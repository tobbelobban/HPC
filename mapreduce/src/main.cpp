#include <stdlib.h>
#include <iostream>
#include <string>
#include <mpi.h>

#define MASTER 0

void read(const char*);
void map();
void reduce();

int world_rank;
int world_size;

int main(int argc, char ** argv) {

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	if (argc < 2)
		return 0;

	read(argv[1]);
	map();
	reduce();

	MPI_Finalize();
	return 0;
}

void read(const char* filename) {

	// Evenly distributed
	MPI_Offset offset;
	MPI_File fh;
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
	MPI_File_get_size(fh, &offset);

	std::cout << offset*world_rank << std::endl;


}

void map() {

}

void reduce() {

}
