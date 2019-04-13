#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <omp.h>

#include "sum.h"

#ifndef TEST_ITER
#define TEST_ITER 20
#endif

#define MY_ASSERT(expr) ( expr ? (void) (0) : printf("%s:%d %s: Assertion failed.\n", __FILE__, __LINE__, __func__) );

double *x = NULL;
int size = 0;

void read_input(char *filename, double **x, int *size);
void generate_random(double**, int);
void serial_sum(double*, int, double*);
void print_usage();
void run_test(char*, int, void (*)(double*), double*);

int main(int argc, char *argv[])
{
	int opt;
	int debug = 0;
	int repeat = 1;
	int num_threads = omp_get_max_threads();

	double omp_sum_val, omp_atomic_sum_val, omp_critical_sum_val, omp_local_sum_val, omp_padded_sum_val, omp_optimized_sum_val;

	while ((opt = getopt(argc, argv, "di:r:n:s:")) != -1) {
		switch (opt) {
			case 'n':
				size = atoi(optarg);
				generate_random(&x, size);
				break;
			case 'i':
				read_input(optarg, &x, &size);
				break;
			case 'd':
				debug = 1;
				break;
			case 'r':
				repeat = atoi(optarg);
				break;
			case 's':
				srand(atoi(optarg));
				break;
			default:
				print_usage(argv[0]);
		}
	}

	if (size == 0 && x == NULL) {
		print_usage(argv[0]);
	}

	fprintf(stderr, "Number of threads: %d, repeat each tests %d times ...\n", num_threads, repeat);

	if (debug == 1) {
		double start_time, end_time;
		double exec_time;
		double serial_sum_val;
		double prev_avg_runtime = 0.0, avg_runtime = 0.0, stddev_runtime = 0.0;

		serial_sum(x, size, &serial_sum_val);

		for (int i = 0; i < repeat; i++) {
			start_time = omp_get_wtime();
			for (int j = 0; j < TEST_ITER; j++) {
				serial_sum(x, size, &serial_sum_val);
			}
			end_time = omp_get_wtime();
			exec_time = (end_time - start_time) / (double)TEST_ITER;

			prev_avg_runtime = avg_runtime;
			avg_runtime = avg_runtime + ( exec_time * 1000 - avg_runtime ) / (i + 1);
			stddev_runtime = stddev_runtime + ( exec_time * 1000 - avg_runtime) * ( exec_time * 1000 - prev_avg_runtime);
		}
		stddev_runtime = sqrt(stddev_runtime / (repeat - 1));
		printf("Max(serial) = %.6f duration = %f ± %f ms\n", serial_sum_val, avg_runtime, stddev_runtime);
	}

	run_test("omp", repeat, omp_sum, &omp_sum_val);
	run_test("omp_critical", repeat, omp_critical_sum, &omp_critical_sum_val);
	run_test("omp_atomic", repeat, omp_atomic_sum, &omp_atomic_sum_val);
	run_test("omp_local", repeat, omp_local_sum, &omp_local_sum_val);
	run_test("omp_padded", repeat, omp_padded_sum, &omp_padded_sum_val);
	run_test("omp_private", repeat, omp_private_sum, &omp_optimized_sum_val);
	run_test("omp_reduction", repeat, omp_reduction_sum, &omp_optimized_sum_val);

	free(x);
}

void print_usage(char *program)
{
	fprintf(stderr, "Usage: %s [-d] [-n size [-s seed]] [-r trials]\n", program);
	exit(1);
}

void serial_sum(double *x, int size, double *sum_ret)
{
	double sum = 0;

	for (int i = 0; i < size; i++){
		sum += x[i];
	}

	*sum_ret = sum;
}

void read_input(char *filename, double **x, int *size)
{
	char line_buffer[256];
	int counter = 0;
	double *input;

	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Cannot open file %s: %s\n", filename, strerror(errno));
		exit(1);
	}

	if (fgets(line_buffer, 256, file) != NULL) {
		*size = atoi(line_buffer);
		input = (double*)calloc(sizeof(double), *size);
		if(input == NULL) {
			fprintf(stderr, "Memory error: %s\n", strerror(errno));
			exit(1);
		}

		while (fgets(line_buffer, 256, file) != NULL) {
			input[counter] = atof(line_buffer);
			counter++;
		}
		*x = input;
	}

	if (*size != counter) {
		fprintf(stderr, "File has less number of lines then specified\n");
		exit(1);
	}

	fclose(file);

}

void generate_random(double **x, int size)
{
	double *input = (double*)calloc(sizeof(double), size);

	for (int i = 0; i < size; i++) {
		input[i] = rand() / (double)(RAND_MAX);
	}

	*x = input;
}

void run_test(char *name, int repeat, void (*func)(double*), double *sum_val)
{
	double start_time, end_time;
	double exec_time;
	double avg_runtime = 0.0, prev_avg_runtime = 0.0, stddev_runtime = 0.0;
	double omp_sum_val = 0.0, serial_sum_val;

	prev_avg_runtime = 0.0; avg_runtime = 0.0; stddev_runtime = 0.0;
	func(&serial_sum_val);

	for (int i = 0; i < repeat; i++) {
		start_time = omp_get_wtime();
		for (int j = 0; j < TEST_ITER; j++) {
			func(&omp_sum_val);
		}
		end_time = omp_get_wtime();
		exec_time = (end_time - start_time) / (double)TEST_ITER;
		//if (!(serial_sum_val == omp_sum_val)) {
		//	fprintf(stderr, "assertion failed: %f\n", omp_sum_val);
		//}
		//assert(serial_sum_val == omp_sum_val);

		prev_avg_runtime = avg_runtime;
		avg_runtime = avg_runtime + ( exec_time * 1000 - avg_runtime ) / (i + 1);
		stddev_runtime = stddev_runtime + ( exec_time * 1000 - avg_runtime) * ( exec_time * 1000 - prev_avg_runtime);
	}
	stddev_runtime = sqrt(stddev_runtime / (repeat - 1));
	printf("Test(%s) = %.6f duration = %f ± %f ms\n", name, omp_sum_val, avg_runtime, stddev_runtime);

	*sum_val = omp_sum_val;
}


