#include "mapreduce.hpp"

void MapReduce::init( const char * filename ) {
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh );	
	MPI_File_get_size( fh, &file_size );
	
	file_size--; // remove 1 byte for eof
	local_buff_size = file_size / world_size;
	remaining_buffer_count = local_buff_size;	
	local_offset = local_buff_size * world_rank;
	
	if(local_buff_size >= READSIZE) {
		read_buffer = new char[READSIZE];
	} else {
		read_buffer = new char[local_buff_size];
	}
}

void MapReduce::read() {
	if(remaining_buffer_count >= READSIZE) {
		MPI_File_read_at( fh, local_offset, read_buffer, READSIZE, MPI_CHAR, MPI_STATUS_IGNORE );
		remaining_buffer_count -= READSIZE;	
	} else {
		MPI_File_read_at( fh, local_offset, read_buffer, remaining_buffer_count, MPI_CHAR, MPI_STATUS_IGNORE );
		remaining_buffer_count = 0;	
	}
}

void MapReduce::write() {

}

void MapReduce::map() {
	const char * delims = " .,\t\n";
	char * token = std::strtok(read_buffer, delims);
	while(token != NULL) {
		q_pairs.push({token,1});
		token = std::strtok(NULL, " .\n\t");
	}
}

void MapReduce::reduce() {
	std::vector<std::map<std::string,int>> buckets(world_size, std::map<std::string,int>());
	std::pair<std::string,int> tmp_pair;
	while( !q_pairs.empty() ) {
		tmp_pair = q_pairs.front();
		uint64_t bucket_idx = std::hash<std::string>{}(tmp_pair.first) % world_size;
		auto found = buckets[bucket_idx].find(tmp_pair.first);
		if(found != buckets[bucket_idx].end()) {
			++buckets[bucket_idx].at(tmp_pair.first);
		} else {
			buckets[bucket_idx].insert(tmp_pair);
		}
		q_pairs.pop();
	} 	
	for(auto i = 0; i < world_size; ++i) {
		for(auto it = buckets[i].begin(); it != buckets[i].end(); ++it) {

		}
	}
}




