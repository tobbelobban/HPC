#include "mapreduce.hpp"

void MapReduce::init( const char * filename ) {
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh );
	MPI_File_get_size( fh, &file_size );

	read_size = file_size / world_size;
	file_offset = world_rank * read_size;
	if(world_rank == world_size-1) {
		read_size += file_size - world_size*read_size; // get the last bytes of file
	}
	remaining_read = read_size;
	if(read_size > READSIZE) {
		read_buffer = new char[READSIZE+sizeof(char)]; // Maximum local read size is 64MB
	} else {
		read_buffer = new char[read_size+sizeof(char)]; // in this case each process does a single read
	}
}

void MapReduce::read() {
	if(remaining_read > READSIZE) {
		MPI_File_read_at( fh, file_offset, read_buffer, READSIZE, MPI_CHAR, MPI_STATUS_IGNORE );
		file_offset += READSIZE;
		remaining_read -= READSIZE;
		read_buffer[READSIZE] = '\0';
	} else {
		MPI_File_read_at( fh, file_offset, read_buffer, remaining_read, MPI_CHAR, MPI_STATUS_IGNORE );
		file_offset = read_size;
		read_buffer[remaining_read] = '\0';
		remaining_read = 0;
	}
}

void MapReduce::write(const char * out_file) {
	auto it = result.begin();
	int wr_size = 0;
	std::string wr_str;
	wr_str.reserve(result.size()*5*sizeof(char));
	int wr_offset = 0;
	while(it != result.end()) {
		wr_size += sizeof(char)*(it->first.size()+std::to_string(it->second).size()+4);
		wr_str += '(' + it->first + ',' + std::to_string(it->second) + ")\n";
		++it;
	}
	if(world_rank == MASTER) {
		if(world_size > 1)
			MPI_Send(&wr_size, 1, MPI_INT, (world_rank+1)%world_size, 0, MPI_COMM_WORLD);
	} else {
		MPI_Recv(&wr_offset, 1, MPI_INT, (world_rank-1 > -1 ? world_rank-1 : world_size-1), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		wr_offset += wr_size;
		if(world_rank != world_size-1)
			MPI_Send(&wr_offset, 1, MPI_INT, (world_rank+1)%world_size, 0, MPI_COMM_WORLD);
	}

	MPI_File out_fh;
	MPI_File_open(MPI_COMM_WORLD, out_file, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_fh);
	MPI_File_write_at(out_fh, wr_offset, wr_str.c_str(), wr_size, MPI_CHAR, MPI_STATUS_IGNORE );
	MPI_File_close(&out_fh);
	MPI_File_close(&fh);
}

void MapReduce::map() {
	const char * delims = "/=<>, .\"\'\t\n";
	char * token = std::strtok(read_buffer, delims);
	while(token != NULL) {
		q_pairs.push({token,1});
		token = std::strtok(NULL, delims);
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
	for(int send_rank = 0; send_rank < world_size; ++send_rank) { // rank i decides what is being sent
		for(int recv_rank = 0; recv_rank < world_size; ++recv_rank) { // rank i might have to send to all ranks
			int num_bcasts = buckets[recv_rank].size();
			auto it = buckets[recv_rank].begin();
			MPI_Bcast( &num_bcasts, 1, MPI_INT, send_rank, MPI_COMM_WORLD );
			for(int bcast_idx = 0; bcast_idx < num_bcasts; ++bcast_idx) {
				std::string str;
				int recv_size;
				char * MPI_buffer;
				if( world_rank == send_rank ) {
					str = it->first;
					MPI_buffer = new char[str.size()+sizeof(char)];
					std::strcpy(MPI_buffer,str.c_str()); // adds null terminator
					recv_size = str.size();
					MPI_Bcast( &recv_size, 1, MPI_INT, world_rank, MPI_COMM_WORLD );
					MPI_Bcast( MPI_buffer, recv_size+sizeof(char), MPI_CHAR, world_rank, MPI_COMM_WORLD );
					++it;
				} else {
					MPI_Bcast( &recv_size, 1, MPI_INT, send_rank, MPI_COMM_WORLD );
					MPI_buffer = new char[recv_size+sizeof(char)];
					MPI_Bcast( MPI_buffer, recv_size+sizeof(char), MPI_CHAR, send_rank, MPI_COMM_WORLD );
					str = MPI_buffer;
				}
				delete MPI_buffer;
				int local_count = 0, global_count;
				auto found = buckets[recv_rank].find(str);
				if(found != buckets[recv_rank].end()) {
					local_count = buckets[recv_rank].at(str);
					if(world_rank != send_rank) {
						buckets[recv_rank].erase(str);
					}
				}
				MPI_Reduce( &local_count, &global_count, 1, MPI_INT, MPI_SUM, recv_rank, MPI_COMM_WORLD );
				if(world_rank == recv_rank) {
					auto found = result.find(str);
					if(found != result.end()) {
						result.at(str) += global_count;
					} else {
						result.insert({str,global_count});
					}
				}
			}
		}
	}
}

void MapReduce::cleanup() {
	delete read_buffer;
}
