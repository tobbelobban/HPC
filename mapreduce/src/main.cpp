#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <mpi.h>

#define MASTER 0

void read(std::string);
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

void read(std::string filename) {

	if(world_rank == MASTER) {
		std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    std::cout << in.tellg() / (world_size - 1) << std::endl;

	}


}

void map() {

}

void reduce() {

}
