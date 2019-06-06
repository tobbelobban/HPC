#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H

#include<cstddef>
#include<fstream>
#include<iostream>
#include<mpi.h>
#include<string>
#include<map>
#include<vector>
#include<cstring>
#include <omp.h>

#define MASTER 0
#define READSIZE 67108864 	// 64 MiB
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
	MPI_Datatype read_type;
	MPI_File fh;
	MPI_File out_fh;
	MPI_Offset file_size;

	uint wordlen = MAXWORDLEN;
	uint64_t world_rank;
	uint64_t world_size;

	int set_world_size;
	int set_world_rank;

	uint64_t file_offset;
	uint64_t read_size;
	uint64_t remaining_read;
	uint buffer_size = READSIZE;
	char * read_buffer;

	std::vector<std::map<std::string,uint>> buckets;
	int * bucket_sizes;
	std::map<std::string,uint> result;

	void init( const char * );
	void read();
	void map();
	void reduce();
	void write( const char * );
	void cleanup();
};

#endif
