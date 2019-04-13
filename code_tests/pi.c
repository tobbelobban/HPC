#include<stdio.h>
#include<omp.h>

static long num_steps = 100000;

void main() {
	int i;
	double x, pi, sum = 0.0;
	double step = 1.0/(double)num_steps;
	double start_time = omp_get_wtime();

	omp_set_num_threads(2);

	#pragma omp parallel for private(x) reduction(+:sum)
	for(i = 0; i < num_steps; ++i) {
		x = (i+0.5)*step;
		sum = sum + 4.0/(1.0+x*x);
	}

	double end_time = omp_get_wtime();
	pi = step*sum;
	printf("pi with %ld steps is %f\n",num_steps,pi);
	printf("Execution time was %f seconds\n",(end_time-start_time));
}
