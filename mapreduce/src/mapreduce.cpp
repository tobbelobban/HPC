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
	} else {
		// data was not evenly divided into 64 MiB chunks
		MPI_File_set_view( fh, 0, MPI_CHAR, MPI_CHAR, "native", MPI_INFO_NULL ); // reset view to entire file
		MPI_File_seek( fh, file_offset, MPI_SEEK_SET );
		MPI_File_read_all( fh, read_buffer, remaining_read, MPI_CHAR, MPI_STATUS_IGNORE );
		read_buffer[remaining_read] = '\0';
		file_offset += remaining_read;
		buffer_size = remaining_read;
		remaining_read = 0;
	}
}

void MapReduce::map() {
	// delimeters for tokenizing words
	const std::string delims ("~;:!#¤?^*[]$_&(){}!#%-+/=<>, .\"\'\t\n");

	int num_threads = omp_get_max_threads();
	int thread_id;
	uint chunk, word_start;
	char * thread_buffer;

	#pragma omp parallel private(thread_id, chunk, word_start, thread_buffer)
	{
		// hash function
		std::hash<std::string> hash;

		// set up each thread environment
		thread_id = omp_get_thread_num();
		chunk = buffer_size / num_threads; // how much data each thread will tokenize
		thread_buffer = read_buffer + chunk*thread_id; // pointer to where each thread reads
		word_start = 0;
		if( thread_id == omp_get_max_threads() - 1 )
			chunk += buffer_size - omp_get_max_threads()*chunk; // last thread gets last bytes in read buffer

		//each thread has a buffer for copying words into
		char word_buffer[MAXWORDLEN+sizeof(char)];
		word_buffer[MAXWORDLEN] = '\0';
		bool found = false;

		for(uint c = 0; c < chunk; ++c) {
			for(unsigned d = 0; d < delims.length(); ++d) {
				if( delims.at(d) == thread_buffer[c] ) {
					found = true; // thread found a delimeter char ( thread now has a word)
					break;
				}
			}
			if(!found && c == chunk - 1) {found = true; ++c;} // if at end of chunk make sure to add last chars
			if( found ) {
				if( word_start == c ) { // thread found multiple delimeter chars next to each other
					++word_start;
				} else {
					if( MAXWORDLEN < c - word_start ) { // word is too big for our MPI struct :(
						word_start = c + 1; 	// next possible word can start after delimeter at index c
						continue;
					} else {
						// Make a local copy of the result
						std::memcpy( word_buffer, thread_buffer + word_start, c - word_start );
						word_buffer[c - word_start] = '\0';
						word_start = c + 1;

						// critical section for inserting words into map
						#pragma omp critical
						{
							int to_bucket = hash(word_buffer) % world_size;
							auto found = buckets[to_bucket].find(word_buffer);
							if( found != buckets[to_bucket].end() ) {
								++buckets[to_bucket].at(word_buffer);
							} else {
								buckets[to_bucket].insert({word_buffer,1});
								++bucket_sizes[to_bucket];
							}
						}
					}
				}
				found = false;
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
	MPI_Alltoallv(sendwords, bucket_sizes, send_displs, type_mapred, recvwords, recv_counts, recv_displs, type_mapred, MPI_COMM_WORLD);

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
	// count for write size, initialize to minimal size
	uint64_t wr_size = sizeof(char)*4*result.size();

	// string to be written to file
	std::string write_string("");

	// reserve approximate chunk of memory for string so we avoid resize
	write_string.reserve( sizeof(char) * (5*result.size()+4) );

	// calculate how much each process will write & make string to write
	auto it = result.begin();
	while(it != result.end()) {
		wr_size += sizeof(char)*( it->first.size()+std::to_string(it->second).size() );
		write_string += '(' + it->first + ',' + std::to_string(it->second) + ")\n";
		++it;
	}

	// find out our offset into write file
	uint64_t write_counts[world_size];
	uint64_t wr_offset = 0;

	MPI_Allgather( &wr_size, 1, MPI_UINT64_T, write_counts, 1, MPI_UINT64_T, MPI_COMM_WORLD );

	for(uint i = 0; i < world_rank; ++i)
		wr_offset += write_counts[i];

	// now we write to file
	MPI_File_open(MPI_COMM_WORLD, out_file, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_fh);
	MPI_File_seek( out_fh, wr_offset, MPI_SEEK_SET);
	MPI_File_write_all( out_fh, write_string.c_str(), write_string.size(), MPI_CHAR, MPI_STATUS_IGNORE );
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
