#include "mapreduce.hpp"

void MapReduce::init( const char * filename ) {
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh );
	MPI_File_get_size( fh, &file_size );

	local_buff_size = file_size / world_size;
	remaining_buffer_count = local_buff_size;
	local_offset = remaining_buffer_count * world_rank;

	if( local_buff_size >= READSIZE ) {
		read_buffer = new char[READSIZE+1];
	} else {
		read_buffer = new char[local_buff_size+1];
	}
}

void MapReduce::read() {
	if(remaining_buffer_count >= READSIZE) {
		MPI_File_read_at( fh, local_offset, read_buffer, READSIZE, MPI_CHAR, MPI_STATUS_IGNORE );
		remaining_buffer_count -= READSIZE;
		local_offset += READSIZE;
	} else {
		MPI_File_read_at( fh, local_offset, read_buffer, remaining_buffer_count, MPI_CHAR, MPI_STATUS_IGNORE );
		local_offset += remaining_buffer_count;
		remaining_buffer_count = 0;
	}
}

void MapReduce::write() {

}

void MapReduce::map() {
	const char * delims = ", .\t\n";
	char * token = std::strtok(read_buffer, delims);
	while(token != NULL) {
		q_pairs.push({token,1});
		std::cout << token << '\n';
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
	for(int send_rank = 0; send_rank < world_size; ++send_rank) { // rank i decides what is being sent
		for(int recv_rank = 0; recv_rank < world_size; ++recv_rank) { // rank i might have to send to all ranks
			int num_bcasts = buckets[recv_rank].size(), local_count = 0, global_count;
			auto it = buckets[recv_rank].begin();
			MPI_Bcast( &num_bcasts, 1, MPI_INT, send_rank, MPI_COMM_WORLD );
			MPI_Barrier( MPI_COMM_WORLD );
			for(int bcast_idx = 0; bcast_idx < num_bcasts; ++bcast_idx) {
				std::string str;
				int recv_size;
				char * MPI_buffer;
				if( world_rank == send_rank ) {
					str = it->first;
					MPI_buffer = new char[str.size()+1];
					std::strcpy(MPI_buffer,str.c_str());
					recv_size = str.size();
					MPI_Bcast( &recv_size, 1, MPI_INT, world_rank, MPI_COMM_WORLD );
					MPI_Bcast( MPI_buffer, recv_size, MPI_CHAR, world_rank, MPI_COMM_WORLD );
					++it;
				} else {
					MPI_Bcast( &recv_size, 1, MPI_INT, send_rank, MPI_COMM_WORLD );
					MPI_buffer = new char[recv_size+1];
					MPI_Bcast( MPI_buffer, recv_size, MPI_CHAR, send_rank, MPI_COMM_WORLD );
					str = MPI_buffer;
				}
				delete MPI_buffer;
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
