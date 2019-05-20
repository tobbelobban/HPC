#include "mapreduce.hpp"

void MapReduce::init() {
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
}

void MapReduce::read(const char * filename) {
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY,
					MPI_INFO_NULL, &fh );
	MPI_File_get_size( fh, &file_size );
	file_size--; // reomve 1 byte for eof

	int local_buff_size = file_size / world_size;

	int local_offset = local_buff_size * world_rank;

	read_buffer = new char[local_buff_size];

	MPI_File_read_at( fh, local_offset, read_buffer, 
						local_buff_size, MPI_CHAR, MPI_STATUS_IGNORE );

}

void MapReduce::write() {

}

void MapReduce::map() {
	const char * delims = " .,\t\n";
	char * token = std::strtok(read_buffer, delims);
	while(token != NULL) {
		pairs.push_back({token,1});
		token = std::strtok(NULL, " .\n\t");
	}
}

void MapReduce::reduce() {

}
