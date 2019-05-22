#include <stdlib.h>
#include <iostream>
#include <string>
#include <mpi.h>
#include <string.h> // strtok
#include <vector>
#include <map>

#define MASTER 0

void read(const char*);
void map(char*);
void reduce(std::vector<std::pair<std::string,uint64_t>> []);

int world_rank;
int world_size;

int main(int argc, char ** argv) {

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	if (argc < 2)
		return 0;

	read(argv[1]);
	MPI_Finalize();
	return 0;
}

void read(const char* filename) {

	// Evenly distributed
	MPI_Offset filesize;
	MPI_File fh;
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
	MPI_File_get_size(fh, &filesize);

	// TODO: Read portions of the input
	int count_buff = (int) filesize / world_size;
	char buf[count_buff];


	MPI_File_read_at(fh, count_buff*world_rank, &buf,
	                 count_buff, MPI_CHAR, MPI_STATUS_IGNORE);

		//std::string res(buf);
		//std::cout << world_rank << ": " << res << std::endl;

	map(buf);

	MPI_File_close(&fh);
}

void map(char* buf) {

	// Buckets for the data
	std::vector<std::pair<std::string,uint64_t>> bucket[world_size];

	// Basic hash function
	std::hash<std::string> hash;


	// Tokenize input

  char * pch;
	char delimiter[] = " .\n\t";
  pch = strtok (buf,delimiter);
  while (pch != NULL)
  {
		// Add the <k,v> pairs to buckets
    bucket[hash(pch) % world_size].push_back({pch,1});
    pch = strtok (NULL, delimiter);
  }

	// TODO: Local reduce

  reduce(bucket);

  return;
}

void reduce(std::vector<std::pair<std::string,uint64_t>> bucket[]) {
	// How much data each bucket contains
	int receive_count[world_size];
	int send_count[world_size];

	// How much data to send to each process
	int i;
	for(i = 0; i < world_size; i++)
			send_count[i] = bucket[i].size();

	std::cout << "Master payload: " <<send_count[MASTER] << " from rank: " << world_rank <<'\n';

	// Send info of how much data each process will recieve from the other
	MPI_Request req[world_size];
	for(i = 0; i < world_size; i++) {
			MPI_Igather(&send_count[i], 1, MPI_INT,
		              receive_count, 1, MPI_INT,
		              i, MPI_COMM_WORLD, &req[i]);
	}

	// Wait until the process know how much data to recieve
	MPI_Wait(&req[world_rank], MPI_STATUS_IGNORE);


	if(world_rank == MASTER) {
		for (i = 0; i < world_size; i++) {
			std::cout << receive_count[i] << " from rank:" << i <<'\n';
		}
	}



	// MPI_Ibcast : All send to each other of how much data they will recieve

	// wait for my bcast

	// gatherv

	MPI_Barrier(MPI_COMM_WORLD);

	// ISend to all

	// https://www.mpich.org/static/docs/latest/www3/MPI_Irecv.html

	// http://mpi.deino.net/mpi_functions/MPI_Wait.html

	// https://www.mpich.org/static/docs/latest/www3/MPI_Waitall.html


	// std::cout << bucket[0][i].first << " " << bucket[0][i].second << " ";

	// TODO: Redistribute the data between the processes from the hash bucket



	// TODO: Reduce

}
