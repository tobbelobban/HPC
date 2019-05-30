#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H

#include<fstream>
#include<iostream>
#include<mpi.h>
#include<string>
#include<map>
#include<vector>
#include<cstring>

#define MASTER 0
#define READSIZE 67108864
#define MAXWORDLEN 300

// struct for MPI
struct WordCount {
	uint local_count;
	char word[MAXWORDLEN];
	WordCount(){};
	WordCount(uint count, const char* c_ptr) : local_count(count) {
		std::strcpy(word,c_ptr);
	}
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
	std::map<std::string,uint> result;
	MPI_File fh;
	MPI_File out_fh;
	MPI_Offset file_size;

	void init(const char *);
	void read();
	void write(const char *);
	void map();
	void reduce();
	void cleanup();
};

#endif
