#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int rank, num_ranks, tkn;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&num_ranks);

	if(rank == 0) {
		tkn = 1123123;
		MPI_Send(&tkn,1,MPI_INT,(rank+1)%num_ranks,0,MPI_COMM_WORLD);
		MPI_Recv(&tkn,1,MPI_INT,(rank-1)%num_ranks,0,MPI_COMM_WORLD,
				MPI_STATUS_IGNORE);
	} else {
	MPI_Recv(&tkn,1,MPI_INT,(rank-1)%num_ranks,0,MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);
	tkn++;
	MPI_Send(&tkn,1,MPI_INT,(rank+1)%num_ranks,0,MPI_COMM_WORLD);
	}
	printf("Rank %d recieved %d\n",rank,tkn);
	MPI_Finalize();

	return 0;
}
