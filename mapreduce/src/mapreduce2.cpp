#include "mapreduce.hpp"

void MapReduce::init( const char * filename ) {
	MPI_Comm_size( MPI_COMM_WORLD, &world_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );

	int count = 2;
	int blocklens[] = {1, MAXWORDLEN};
	MPI_Aint disp_array[] = { offsetof(WordCount, local_count), offsetof(WordCount, word)};
	MPI_Datatype type_array[2] = {MPI_INT, MPI_CHAR};
	MPI_Aint lb, extent;
	MPI_Datatype oldtype;
	MPI_Type_create_struct(count, blocklens, disp_array, type_array, &oldtype);
	MPI_Type_get_extent(oldtype, &lb, &extent);
	MPI_Type_create_resized(oldtype, lb, extent, &type_mapred);
	MPI_Type_commit(&type_mapred);

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
	token_v = std::vector<std::string>();
	const char * delims = "/=<>, .\"\'\t\n";
	char * token = std::strtok(read_buffer, delims);
	while(token != NULL) {
		auto len = std::strlen(token);
		if(len >= MAXWORDLEN) {
			token = std::strtok(NULL, delims);
			continue;
		}
		token_v.push_back(token);
		token = std::strtok(NULL, delims);
	}
}

void MapReduce::reduce() {
	std::vector<std::map<std::string,uint>> buckets(world_size, std::map<std::string,uint>());
	int bucket_sizes[world_size] = {0};

	// local reduce
	for(uint i = 0; i < token_v.size(); ++i) {
		uint64_t to_bucket = std::hash<std::string>{}(token_v[i]) % world_size;
		auto found = buckets[to_bucket].find(token_v[i]);
		if( found != buckets[to_bucket].end() ) {
			++buckets[to_bucket].at(token_v[i]);
		} else {
			buckets[to_bucket].insert({token_v[i],1});
			++bucket_sizes[to_bucket];
		}
	}

	//global reduce
	int recv_counts[world_size];
	int send_displs[world_size];
	int recv_displs[world_size];
	int total_recv = 0;
	int total_send = 0;

	MPI_Alltoall( bucket_sizes, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD );

	for(int i = 0; i < world_size; ++i) {
		recv_displs[i] = total_recv;
		send_displs[i] = total_send;
		total_recv += recv_counts[i];
		total_send += bucket_sizes[i];
	}

	WordCount * recvwords = new WordCount[total_recv];
	WordCount * sendwords = new WordCount[total_send];

	int j = 0;

	MPI_Barrier(MPI_COMM_WORLD);
	for(int i = 0; i < world_size; ++i) {
		for(auto it = buckets[i].begin(); it != buckets[i].end(); ++it) {
			sendwords[j] = WordCount(it->second, it->first.c_str());
			++j;

		}
	}


	MPI_Alltoallv(sendwords, bucket_sizes, send_displs, type_mapred, recvwords, recv_counts, recv_displs, type_mapred, MPI_COMM_WORLD);

	for(int i = 0; i < total_recv; ++i) {
		std::string recv_str(recvwords[i].word);
		auto found = result.find(recv_str);
		if(found != result.end()) {
			++result.at(recv_str);
		} else {
			result.insert({recv_str,recvwords[i].local_count});
		}
	}

	delete recvwords;
	delete sendwords;
}


void MapReduce::cleanup() {
	delete read_buffer;
}
