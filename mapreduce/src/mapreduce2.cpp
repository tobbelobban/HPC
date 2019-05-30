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
	std::cout << "here " << world_rank << '\n';
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
	int bucket_sizes[world_size] = {0};
	while( !q_pairs.empty() ) {
		tmp_pair = q_pairs.front();
		uint64_t bucket_idx = std::hash<std::string>{}(tmp_pair.first) % world_size;
		auto found = buckets[bucket_idx].find(tmp_pair.first);
		if(found != buckets[bucket_idx].end()) {
			++buckets[bucket_idx].at(tmp_pair.first);
		} else {
			buckets[bucket_idx].insert(tmp_pair);
			++bucket_sizes[bucket_idx];
		}
		q_pairs.pop();
	}

	int recv_count;
	MPI_Request reduce_reqs[world_size];
	for(int i = 0; i < world_size; ++i)
		MPI_Ireduce(&bucket_sizes[i], &recv_count, 1, MPI_INT, MPI_SUM, i, MPI_COMM_WORLD, &reduce_reqs[i]);
	MPI_Wait( &reduce_reqs[world_rank], MPI_STATUS_IGNORE );
	MPI_Request * send_reqs[world_size];
	for(int recv_rank = 0; recv_rank < world_size; ++recv_rank) {
		int count = 0;
		send_reqs[recv_rank] = new MPI_Request[bucket_sizes[recv_rank]];

		auto it = buckets[recv_rank].begin();
		while(it != buckets[recv_rank].end()) {
			MPI_Isend(it->first.c_str(), it->first.size(), MPI_CHAR, recv_rank, it->second, MPI_COMM_WORLD, &send_reqs[recv_rank][count]);
			++count;
			++it;
		}
	}

	int count = 0, recv_size;

	while(count < recv_count) {
		MPI_Status sts;
		MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &sts);
		MPI_Get_count(&sts, MPI_CHAR, &recv_size);
		char * tmp_buff = new char[recv_size+sizeof(char)];
		tmp_buff[recv_size] = '\0';
		MPI_Recv(tmp_buff, recv_size, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		std::string str(tmp_buff);
		delete tmp_buff;
		auto found = result.find(str);
		if(found != result.end()) {
			result.at(str) += sts.MPI_TAG;
		} else {
			result.insert({str,sts.MPI_TAG});
		}
		++count;
	}
}

void MapReduce::cleanup() {
	delete read_buffer;
}
