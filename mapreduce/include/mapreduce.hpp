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
#define MAXWORDLEN 100

struct WordCount {
	uint local_count;
	char word[MAXWORDLEN];
};

class MapReduce {
public:

	MPI_Datatype type_mapred;

	int world_rank;
	int world_size;

	uint64_t file_offset;
	uint64_t read_size;
	uint64_t remaining_read;
	char * read_buffer;

	std::vector<std::string> token_v;

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
