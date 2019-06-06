#include "mapreduce.hpp"

void MapReduce::init( const char * filename ) {
	// get world size and rank
	MPI_Comm_size( MPI_COMM_WORLD, &set_world_size );
	MPI_Comm_rank( MPI_COMM_WORLD, &set_world_rank );

	world_size = set_world_size;
	world_rank = set_world_rank;

	// create own MPI Dataype for message passing
	MPI_Datatype oldtype;
	int count = 2;
	int blocklens[] = {1, MAXWORDLEN};
	MPI_Aint disp_array[] = { offsetof(WordCount, local_count), offsetof(WordCount, word)};
	MPI_Datatype type_array[2] = {MPI_INT, MPI_CHAR};
	MPI_Aint lb, extent;
	MPI_Type_create_struct(count, blocklens, disp_array, type_array, &oldtype);
	MPI_Type_get_extent(oldtype, &lb, &extent);
	MPI_Type_create_resized(oldtype, lb, extent, &type_mapred);
	MPI_Type_commit(&type_mapred);

	// get readfile size
	MPI_File_open( MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh );
	MPI_File_get_size( fh, &file_size );

	// calculate local read size
	read_size = file_size / world_size;
	file_offset = world_rank * read_size;
	if(world_rank == world_size-1) {
		read_size += file_size - world_size*read_size; // if not even, last process gets the last bytes of file
	}
	remaining_read = read_size;

	// maximum read size is 64 MiB (64 * 1024 * 1024)
	read_buffer = new char[sizeof(char)*READSIZE+sizeof(char)];
	read_buffer[READSIZE] = '\0';

	// create type for reading
	MPI_Datatype chunk_type;
	MPI_Type_contiguous( READSIZE, MPI_CHAR, &chunk_type );
	MPI_Type_create_resized( chunk_type, 0, world_size*READSIZE, &read_type );
	MPI_Type_commit( &read_type );
	MPI_File_set_view( fh, READSIZE*world_rank, MPI_CHAR, read_type, "native", MPI_INFO_NULL );

	// bucket for placing data
	std::vector<std::map<std::string,uint>> tmp_buckets(world_size, std::map<std::string,uint>());
	buckets = tmp_buckets;

	// array of bucket sizes
	bucket_sizes = new int[world_size];
	std::fill(bucket_sizes,bucket_sizes+world_size, 0);
}

void MapReduce::read() {
	if(remaining_read >= READSIZE) {
		MPI_File_read_all( fh, read_buffer, 1, read_type, MPI_STATUS_IGNORE );
		remaining_read -= READSIZE;
		file_offset += READSIZE;
		map_read = READSIZE;
	} else {
		MPI_File_set_view( fh, 0, MPI_CHAR, MPI_CHAR, "native", MPI_INFO_NULL );
		MPI_File_seek( fh, file_offset, MPI_SEEK_SET );
		MPI_File_read_all( fh, read_buffer, remaining_read, MPI_CHAR, MPI_STATUS_IGNORE );
		read_buffer[remaining_read] = '\0';
		file_offset += remaining_read;
		map_read = remaining_read;
		remaining_read = 0;
	}
}

/*
void MapReduce::map() {
	// delims is an array of chars we wish to split strings by
	const char * delims = "~;:!#¤?^*[]$_&(){}!#%-+/=<>, .\"\'\t\n";
	char * token = std::strtok(read_buffer, delims);

	// get all words according to delims
	while(token != NULL) {
		auto len = std::strlen(token);
		// we do not allowed words longer than wordlen
		if(len > wordlen) {
			token = std::strtok(NULL, delims);
			continue;
		}
		uint64_t to_bucket = std::hash<std::string>{}(token) % world_size;
		// check if we already found current word
		auto found = buckets[to_bucket].find(token);
		if( found != buckets[to_bucket].end() ) {
			++buckets[to_bucket].at(token);
		} else {
			buckets[to_bucket].insert({token,1});
			++bucket_sizes[to_bucket];
		}
		token = std::strtok(NULL, delims);
	}
}
*/

void MapReduce::map() {
	// Basic hash function
	std::hash<std::string> hash;

	std::string delims ("~;:!#¤?^*[]$_&(){}!#%-+/=<>, .\"\'\t\n");

	std::vector<std::vector<std::string>> words;

	#pragma omp parallel
	{
		char * str = read_buffer;

		bool found = false;

		int thread_id = omp_get_thread_num();
		int num_threads = omp_get_num_threads();
		int chunk = map_read / num_threads;
		int from = chunk*thread_id;
		int to = chunk*thread_id;

		if(thread_id == 0) {
			for(int i = 0; i < num_threads; i++)
			words.push_back(std::vector<std::string>());
		}

		#pragma omp barrier


		char res[MAXWORDLEN];

		// TODO: Optional, move pointer to the right if intervening word.

		for(int j = chunk*thread_id; j < chunk * (thread_id + 1) && j < map_read; j++) {

			for(unsigned i = 0; i < delims.length(); ++i) {
				if( delims.at(i) == str[j] ) {
					found = true;
					break;
				}
			}

			if (found) {
				if(from == to) {
					from++;
					to++;
				} else {

					if(MAXWORDLEN < to - from) {
						to++;
						from = to;
						continue;
					} else {

						// Make a local copy of the result
						for(int r = 0; r < to-from; r++)
							res[r] = str[from+r];

							res[to-from] = '\0';
							to++;
							from = to;

							// Add to bucket
							std::string s(res);
							//#pragma omp critical
							words[thread_id].push_back(s);

						}
					}
				} else {
					to++;
				}
				found = false;
			}
	}

for(int i = 0; i < words.size(); i++) {
	for(int j = 0; j < words[i].size(); j++) {
		// check if we already found current word
		int to_bucket = (int) hash(words[i][j]) % world_size;
		auto found = buckets[to_bucket].find(words[i][j]);
		if( found != buckets[to_bucket].end() ) {
			++buckets[to_bucket].at(words[i][j]);
		} else {
			buckets[to_bucket].insert({words[i][j],1});
			++bucket_sizes[to_bucket];
		}


	}
}

}


void MapReduce::reduce() {
	//global reduce via alltoall/v
	int recv_counts[world_size];
	int send_displs[world_size];
	int recv_displs[world_size];
	int total_recv = 0;
	int total_send = 0;

	// find out how many words are being sent to each process
	MPI_Alltoall( bucket_sizes, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD );



	// prepare displacement arrays for mpi_alltoallv
	for(uint i = 0; i < world_size; ++i) {
		recv_displs[i] = total_recv;
		send_displs[i] = total_send;
		total_recv += recv_counts[i];
		total_send += bucket_sizes[i];
	}
	// send & recieve buffers
	WordCount * recvwords = new WordCount[total_recv];
	WordCount * sendwords = new WordCount[total_send];

	int word_count = 0;
	for(uint i = 0; i < world_size; ++i) {
		for(auto it = buckets[i].begin(); it != buckets[i].end(); ++it) {
			sendwords[word_count] = WordCount(it->second, it->first.c_str());
			++word_count;
		}
	}

	// this is our magical line of code <3
	MPI_Alltoallv(sendwords, bucket_sizes, send_displs, type_mapred, recvwords, recv_counts, recv_displs, type_mapred,
MPI_COMM_WORLD);

	// finally compute total counts of all words
	for(int i = 0; i < total_recv; ++i) {
		std::string recv_str(recvwords[i].word);
		auto found = result.find(recv_str);
		if(found != result.end()) {
			++result.at(recv_str);
		} else {
			result.insert({recv_str,recvwords[i].local_count});
		}
	}
	// we don't need these buffers any more :)
	delete recvwords;
	delete sendwords;
}

void MapReduce::write(const char * out_file) {
	// counts how many chars will be written
	uint64_t wr_size = 0;

	// calculate how much each process will write
	auto it = result.begin();
	while(it != result.end()) {
		wr_size += sizeof(char)*(it->first.size()+std::to_string(it->second).size()+4);
		++it;
	}

	// find out our offset into write file
	uint64_t write_counts[world_size];
	uint64_t wr_offset = 0;
	write_counts[world_rank] = wr_size;
	auto res = MPI_Allgather( &wr_size, 1, MPI_UINT64_T, write_counts, 1, MPI_UINT64_T, MPI_COMM_WORLD );

	for(uint i = 0; i < world_rank; ++i)
		wr_offset += write_counts[i];

	// now we write to file
	MPI_File_open(MPI_COMM_WORLD, out_file, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_fh);
	MPI_File_seek( out_fh, wr_offset, MPI_SEEK_SET );

	it = result.begin();
	std::string write_string;
	while( it != result.end() ) {
		write_string = '(' + it->first + ',' + std::to_string(it->second) + ")\n";
		MPI_File_write( out_fh, write_string.c_str(), write_string.size(), MPI_CHAR, MPI_STATUS_IGNORE );
		++it;
	}
}

void MapReduce::cleanup() {
	// let's tidy up so that valgrind is happy
	delete read_buffer;
	delete bucket_sizes;

	// and close our files
	MPI_File_close(&out_fh);
	MPI_File_close(&fh);

	MPI_Type_free( &type_mapred );
	MPI_Type_free( &read_type );
}
