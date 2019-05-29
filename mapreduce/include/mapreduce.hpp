#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H

#include<fstream>
#include<iostream>
#include<mpi.h>
#include<math.h>
#include<string>
#include<map>
#include<vector>
#include<queue>
#include<cstring>
#include <sstream>

#define MASTER 0
#define READSIZE 64000000

class MapReduce {
public:
	int world_rank;
	int world_size;
	int file_offset;
	int read_size;
	int remaining_read;
	char * read_buffer;
	std::queue<std::pair<std::string,int> > q_pairs;
	std::map<std::string,int> result;
	MPI_File fh;
	MPI_Offset file_size;

	void init(const char *);
	void read();
	void write(const char *);
	void map();
	void reduce();
	void cleanup();
};

#endif
