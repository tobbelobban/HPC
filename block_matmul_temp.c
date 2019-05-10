#include "block_matmul.h"
#define MASTER 0


// mpirun -n 4 ./bin/matmul.out dat/A dat/B out.txt

struct Config {
	/* MPI Files */
	MPI_File A_file, B_file, C_file;
	char *outfile;

	/* MPI Datatypes for matrix blocks */
	MPI_Datatype block;

	/* Matrix data */
	double *A, *A_tmp, *B, *C;

	/* Cart communicators */
	MPI_Comm grid_comm;
	MPI_Comm row_comm;
	MPI_Comm col_comm;

	/* Cart communicator dim and ranks */
	int dim[2], coords[2];
	int world_rank, world_size, grid_rank;
	int row_rank, row_size, col_rank, col_size;

	/* Full matrix dim */
	int A_dims[2];
	int B_dims[2];
	int C_dims[2];
	int matrix_size;

	/* Process local matrix dim */
	int local_dims[2];
	int local_size;
};

struct Config config;

void init_matmul(char *A_file, char *B_file, char *outfile)
{

	MPI_Comm_rank(MPI_COMM_WORLD, &config.world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &config.world_size);

	/* Copy output file name to configuration */
	config.outfile = outfile;

	if(config.world_rank == MASTER) {
		/* Get matrix size header */
		int buf[2];
		MPI_File_open(MPI_COMM_SELF, A_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &config.A_file);
		MPI_File_read(config.A_file, buf, 2, MPI_INT, MPI_STATUS_IGNORE);
		config.A_dims[0] = buf[0];
		config.A_dims[1] = buf[1];
		MPI_File_close(&config.A_file);

		MPI_File_open(MPI_COMM_SELF, B_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &config.B_file);
		MPI_File_read(config.B_file, buf, 2, MPI_INT, MPI_STATUS_IGNORE);
		config.B_dims[0] = buf[0];
		config.B_dims[1] = buf[1];
		MPI_File_close(&config.B_file);
	}

	/* Broadcast global matrix sizes */

	MPI_Bcast(config.A_dims, 2, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(config.B_dims, 2, MPI_INT, MASTER, MPI_COMM_WORLD);
	//printf("A_Matrix: %d\n", config.B_dims[0]);

	/* Set dim of tiles relative to the number of processes as NxN where N=sqrt(world_size) */
	int N = sqrt(config.world_size);
	config.dim[0] = N;
	config.dim[1] = N;

	/* Verify dim of A and B matches for matul and both are square*/
	if(config.A_dims[0] != config.A_dims[1] || config.B_dims[0] != config.B_dims[1] || config.A_dims[1] != config.B_dims[0])
		return;

	//printf("A_Matrix: %d\n", config.B_dims[0]);
	/* Create Cart communicator for NxN processes */
	int grid_period[2] = {0};
	MPI_Cart_create(MPI_COMM_WORLD, 2, config.dim, grid_period, 0, &config.grid_comm);
	MPI_Comm_rank(config.grid_comm, &config.grid_rank);
	MPI_Cart_coords(config.grid_comm, config.grid_rank, 2, config.coords);


	int remain[2];
	/* Sub div cart communicator to N row communicator */
	remain[0] = 1;
	remain[1] = 0;
	MPI_Cart_sub(config.grid_comm, remain, &config.row_comm);
	MPI_Comm_rank(config.row_comm, &config.row_rank);
	MPI_Comm_size(config.row_comm, &config.row_size);

	/* Sub div cart communicator to N col communicator */
	remain[0] = 0;
	remain[1] = 1;
	MPI_Cart_sub(config.grid_comm, remain, &config.col_comm);
	MPI_Comm_rank(config.col_comm, &config.col_rank);
	MPI_Comm_size(config.col_comm, &config.col_size);

	/* Setup sizes of full matrices */
	config.C_dims[0] = config.A_dims[0];
	config.C_dims[1] = config.B_dims[1];
	config.matrix_size = config.C_dims[0]*config.C_dims[1];

	//printf("%d %d\n", config.coords[0], config.coords[1]);
	//printf("%d %d\n", config.row_rank, config.col_rank);



	/* Setup sizes of local matrix tiles */

	config.local_dims[0] = config.A_dims[0] / N;
	config.local_dims[1] = config.A_dims[0] / N;
	config.local_size = config.local_dims[0]*config.local_dims[1];

	/* Create subarray datatype for local matrix tile */
	int starts[2];
	starts[0] = config.local_dims[0]*config.coords[0];
	starts[1] =	config.local_dims[1]*config.coords[1];
	MPI_Type_create_subarray(2, config.A_dims, config.local_dims, starts, MPI_ORDER_C, MPI_DOUBLE, &config.block);
  MPI_Type_commit(&config.block);

	/* Create data array to load actual block matrix data */
	// MALLOC for each thing in congig

	config.A = (double *) malloc(sizeof(double)*config.local_size);
	config.A_tmp = (double *) malloc(sizeof(double)*config.local_size);
	config.B = (double *) malloc(sizeof(double)*config.local_size);
	config.C = (double *) calloc(config.local_size,sizeof(double));

	// A_File

	/* Set fileview of process to respective matrix block */
	MPI_File_open(MPI_COMM_WORLD, A_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &config.A_file);
	MPI_File_open(MPI_COMM_WORLD, B_file, MPI_MODE_RDONLY, MPI_INFO_NULL, &config.B_file);

	MPI_Offset disp = 2 * sizeof(int);
	MPI_File_set_view(config.A_file, disp, MPI_DOUBLE, config.block, "native", MPI_INFO_NULL);
	MPI_File_set_view(config.B_file, disp, MPI_DOUBLE, config.block, "native", MPI_INFO_NULL);

	/* Collective read blocks from files */
	MPI_File_read(config.A_file, config.A, config.local_size, MPI_DOUBLE, MPI_STATUS_IGNORE);
	MPI_File_read(config.B_file, config.B, config.local_size, MPI_DOUBLE, MPI_STATUS_IGNORE);

	/* Close data source files */
	MPI_File_close(&config.A_file);
	MPI_File_close(&config.B_file);

	memcpy(config.A_tmp, config.A, sizeof(double)*config.local_size);

	//printf(" (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.A[0], config.A[1], config.A[2], config.A[3]);
	//printf(" (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.A_tmp[0], config.A_tmp[1], config.A_tmp[2], config.A_tmp[3]);
}

void cleanup_matmul()
{
	/* Rank zero writes header specifying dim of result matrix C */
	MPI_File_open( MPI_COMM_WORLD, config.outfile, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &config.C_file);
	/* Set fileview of process to respective matrix block with header offset */
	if(MASTER == config.world_rank) {
		MPI_File_write_at(config.C_file, 0, config.C_dims, 2, MPI_INT, MPI_STATUS_IGNORE);
	}

	MPI_Offset disp = 2 * sizeof(int);
	MPI_File_set_view(config.C_file, disp, MPI_DOUBLE, config.block, "native", MPI_INFO_NULL);

	/* Collective write and close file */
	MPI_File_read(config.C_file, config.C, config.local_size, MPI_DOUBLE, MPI_STATUS_IGNORE);
	MPI_File_close(&config.B_file);
	/* Cleanup */
	free(config.A);
	free(config.A_tmp);
	free(config.B);
	free(config.C);
}

void compute_fox()
{

	int i, src_vert, dest_vert;
	/* Compute source and target for verticle shift of B blocks */
	MPI_Cart_shift(config.row_comm, 0, -1, &src_vert, &dest_vert);
	//printf("Beginning: (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.B[0], config.B[1], config.B[2], config.B[3]);
	//printf("%d %d \n", config.col_rank, dest_vert);

	for (i = 0; i < config.dim[0]; i++) {
		/* Diag + i broadcast block A horizontally and use A_tmp to preserve own local A */
		int bcast = (int) (config.col_rank + i) % config.local_dims[1];

		if(bcast == config.row_rank)
			MPI_Bcast(config.A_tmp, config.local_size, MPI_DOUBLE, bcast, config.row_comm);
		else
				MPI_Bcast(config.A, config.local_size, MPI_DOUBLE, bcast, config.row_comm);


		//MPI_Bcast(config.A, config.local_size, MPI_DOUBLE, bcast, config.row_comm);

		//printf(" (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.A[0], config.A[1], config.A[2], config.A[3]);

		/* dgemm with blocks */
    int m = config.local_dims[0], n = config.local_dims[0], k = config.local_dims[0];
    double alpha = 1.0, beta = 1.0;

		if(bcast == config.row_rank)
			cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, n, k, alpha, config.A_tmp, k, config.B, n, beta, config.C, n);
		else
			cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
							m, n, k, alpha, config.A, k, config.B, n, beta, config.C, n);

/*
		printf("A: (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.A[0], config.A[1], config.A[2], config.A[3]);
		printf("B: (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.B[0], config.B[1], config.B[2], config.B[3]);
		printf("Res: (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.C[0], config.C[1], config.C[2], config.C[3]);
*/
		/* Shfting block B upwards and receive from process below */

		MPI_Sendrecv_replace(config.B, config.local_size, MPI_DOUBLE, dest_vert, 0, src_vert, 0, config.row_comm, MPI_STATUS_IGNORE);
		//printf(" (%d, %d) %lf %lf %lf %lf\n", config.coords[0],config.coords[1], config.B[0], config.B[1], config.B[2], config.B[3]);

		// Reset A
		//memcpy(config.A, config.A_tmp, sizeof(double)*config.local_size);

	}
}
