#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H

#include<fstream>
#include<iostream>
#include<mpi.h>
#include<math.h>
#include<string>
#include<vector>
#include<cstring>

#define MASTER 0

class MapReduce {
public:
	int world_rank;
	int world_size;
	char * read_buffer;
	std::vector<std::pair<std::string,uint64_t>> pairs;
	MPI_File fh;
	MPI_Offset file_size;

	void init();
	void read(const char *);
	void write();
	void map();
	void reduce();
};

#endif
