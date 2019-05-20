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
	//map();
	reduce();

	MPI_Finalize();
	return 0;
}

void read(const char* filename) {

	// Evenly distributed
	MPI_Offset filesize;
	MPI_File fh;
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
	MPI_File_get_size(fh, &filesize);

	//std::cout << filesize*world_rank << std::endl;



	// TODO: Read portions of the input
	int count_buff = (int) filesize / world_size;
	char buf[count_buff];
	MPI_File_read_at(fh, count_buff*world_rank, &buf,
                    count_buff, MPI_CHAR, MPI_STATUS_IGNORE);

	//std::string res(buf);
	//std::cout << world_rank << ": " << res << std::endl;

	if(world_rank == MASTER)
		map(buf);

	// TODO: Tokenize the read buffer in the map function

	// TODO: hash the word and put into bucket

}

void map(char* buf) {

	std::vector<std::pair<std::string,uint64_t>> v;

	//std::pair<std::string,uint64_t> PAIR1("hello", 1);
	//std::cout << PAIR1.first << " " << PAIR1.second << std::endl;


	// Tokenize input

  char * pch;
	char delimiter[] = " .\n\t";
  //printf ("Splitting string \"%s\" into tokens:\n",buf);
  pch = strtok (buf,delimiter);
  while (pch != NULL)
  {
    v.push_back({pch,1});
    pch = strtok (NULL, delimiter);
  }


	for (int i = 0; i < v.size(); ++i)
    {
        std::cout << v[i].first << " " << v[i].second << " ";
    }



  return;
}

void reduce() {

}
