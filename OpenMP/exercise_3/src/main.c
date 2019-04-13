#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <omp.h>

#include "dft.h"

// this for the rounding error, increasing N rounding error increases
// 0.01 precision good for N > 8000
#define R_ERROR 0.01

int check_results(double*, double*, double*, double*, int N);
int set_output_zero(double*, double*, int N);
void read_input(char*, double**, double**, int*);
void serial_findmax(double*, int, double*, int*);
void print_usage();

int main(int argc, char *argv[])
{
	int opt;

	int input_size = 0;
	int prev_size = -1;
	int repeat = 1;

	double *real_input = NULL, *imag_input = NULL;
	double *real_forward = NULL, *imag_forward = NULL;
	double *real_backward = NULL, *imag_backward = NULL;
	double *real_truth = NULL, *imag_truth = NULL;

	double start_time, end_time;
	double avg_runtime = 0.0, prev_avg_runtime = 0.0, stddev_runtime = 0.0;

	while ((opt = getopt(argc, argv, "dv:i:r:")) != -1) {
		switch (opt) {
			case 'i':
				if (input_size != 0) prev_size = input_size;
				read_input(optarg, &real_input, &imag_input, &input_size);
				if (prev_size != -1 && input_size != prev_size) fprintf(stderr, "Error: problem and validation list should have same size.\n");
				break;
			case 'v':
				if (input_size != 0) prev_size = input_size;
				read_input(optarg, &real_truth, &imag_truth, &input_size);
				if (prev_size != -1 && input_size != prev_size) fprintf(stderr, "Error: problem and validation list should have same size.\n");
				break;
			case 'r':
				repeat = atoi(optarg);
				break;
			default:
				print_usage(argv[0]);
		}
	}

	if (input_size == 0 || real_input == NULL || imag_input == NULL) {
		print_usage(argv[0]);
	}

	real_forward = (double*)calloc(sizeof(double), input_size);
	imag_forward = (double*)calloc(sizeof(double), input_size);
	real_backward = (double*)calloc(sizeof(double), input_size);
	imag_backward = (double*)calloc(sizeof(double), input_size);
	if (real_forward == NULL || imag_forward == NULL || real_backward == NULL || imag_backward == NULL) {
		fprintf(stderr, "Memory error: %s\n", strerror(errno));
		exit(1);
	}

	for (int i = 0; i < repeat; i++) {
		start_time = omp_get_wtime();
		DFT(1, real_input, imag_input, real_forward, imag_forward, input_size);
		DFT(-1, real_forward, imag_forward, real_backward, imag_backward, input_size);
		end_time = omp_get_wtime();

		if (real_truth != NULL && imag_truth != NULL) {
			check_results(real_truth, imag_truth, real_forward, imag_forward, input_size);
			check_results(real_input, imag_input, real_backward, imag_backward, input_size);
		}

		set_output_zero(real_forward, imag_forward, input_size);
		set_output_zero(real_backward, imag_backward, input_size);

		prev_avg_runtime = avg_runtime;
		avg_runtime = avg_runtime + ( (end_time - start_time) - avg_runtime ) / (i + 1);
		stddev_runtime = stddev_runtime + ( (end_time - start_time) - avg_runtime) * ( (end_time - start_time) - prev_avg_runtime);
	}
	stddev_runtime = sqrt(stddev_runtime / (repeat - 1));
	printf("duration\t= %f±%f s\n", avg_runtime, stddev_runtime);

	if (real_truth != NULL && imag_truth != NULL) {
		free(real_truth);
		free(imag_truth);
	}
	free(real_input);
	free(imag_input);
	free(real_forward);
	free(imag_forward);
	free(real_backward);
	free(imag_backward);
}

// check if x = IDFT(DFT(x))
int check_results(double* xr, double* xi, double* xr_check, double* xi_check, int N)
{
	// x[0] and x[1] have typical rounding error problem
	// interesting there might be a theorem on this
	for (int n = 0; n < N;n++) {
		//printf("%f %f + %f %f\n", xr[n], xr_check[n], xi[n], xi_check[n]);
		if (fabs(xr[n] - xr_check[n]) > R_ERROR)
			printf("ERROR - x[%d] = %f, inv(X)[%d]=%f \n",n,xr[n], n,xr_check[n]);
		if (fabs(xi[n] - xi_check[n]) > R_ERROR)
			printf("ERROR - x[%d] = %f, inv(X)[%d]=%f \n",n,xi[n], n,xi_check[n]);
	}
	return 1;
}

// set to zero the output vector
int set_output_zero(double* Xr_o, double* Xi_o, int N)
{
	for (int n = 0; n < N; n++) {
		Xr_o[n] = 0.0;
		Xi_o[n] = 0.0;
	}
	return 1;
}

void print_usage(char *program)
{
	fprintf(stderr, "Usage: %s [-v file name] [-i file name] [-r trials]\n", program);
	exit(1);
}

void read_input(char *filename, double **xr, double **xi, int *size)
{
	char line_buffer[256];
	int counter = 0;
	double *real_input, *imag_input;
	double real, imag;

	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Cannot open file %s: %s\n", filename, strerror(errno));
		exit(1);
	}

	if (fgets(line_buffer, 256, file) != NULL) {
		*size = atoi(line_buffer);
		real_input = (double*)calloc(sizeof(double), *size);
		imag_input = (double*)calloc(sizeof(double), *size);
		if(real_input == NULL || imag_input == NULL) {
			fprintf(stderr, "Memory error: %s\n", strerror(errno));
			exit(1);
		}
	}

	while (fgets(line_buffer, 256, file) != NULL) {
		sscanf(line_buffer, "%lf %lf", &real, &imag);
		real_input[counter] = real;
		imag_input[counter] = imag;
		counter++;
	}

	if (*size != counter) {
		fprintf(stderr, "File has less number of lines then specified\n");
		exit(1);
	}

	fclose(file);

	*xr = real_input;
	*xi = imag_input;
}
