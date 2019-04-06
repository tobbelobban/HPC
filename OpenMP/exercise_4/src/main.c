#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <errno.h>
#include <omp.h>

#include "nbody.h"

void print_usage(const char*e);
void generate_particles(struct Body**t, int);
void read_input(struct Body**, int*, const char*);

int main(int argc, char *argv[])
{
	int opt;

	struct Body *bodies = NULL;
	int N;
	double G = 1.0;
	double DT = 1.0;
	double EPS = 0.1;
	int steps = 100;

	int output_steps = 0;

	while ((opt = getopt(argc, argv, "n:i:o:s:d:e:G:")) != -1) {
		switch (opt) {
			case 'e':
				EPS = atof(optarg);
				break;
			case 'n':
				if (bodies == NULL) {
					N = atoi(optarg);
					generate_particles(&bodies, atoi(optarg));
				}
				break;
			case 'i':
				if (bodies == NULL) {
					printf("Input: %s\n", optarg);
					read_input(&bodies, &N, optarg);
				}
				break;
			case 'o':
				output_steps = atoi(optarg);
				break;
			case 's':
				steps = atoi(optarg);
				break;
			case 'd':
				DT = atof(optarg);
				break;
			case 'G':
				G = atof(optarg);
				break;
			default:
				print_usage(argv[0]);
		}
	}

	if (bodies == NULL) {
		N = 1000;
		generate_particles(&bodies, 1000);
	}

	printf("N       = %d\n", N);
	printf("G       = %e\n", G);
	printf("dt      = %e\n", DT);
	printf("EPS     = %e\n", EPS);
	printf("time    = %d\n", steps);
	printf("Threads = %d\n", omp_get_max_threads());
	printf("checkpoints: every %d steps\n", output_steps);

	double start, stop;
	start = omp_get_wtime();
	nbody(bodies, steps, output_steps, N, G, DT, EPS);
	stop = omp_get_wtime();
	printf("Total runtime: %f\n", stop - start);

	return 0;
}

void print_usage(const char *name)
{
	printf("Usage: %s [-i input file / -n size (1000)] [-o steps per output (0)] [-d dt (1.0)] [-G Gravitational constant (1.0)] [-e softening (0.1)] [-s time steps (100)] \n", name);
	exit(1);
}

void generate_particles(struct Body **ret, int size)
{
	struct Body *bodies = (struct Body*)calloc(sizeof(struct Body), size);

	if (bodies == NULL) {
		fprintf(stderr, "Memory error: %s\n", strerror(errno));
		exit(1);
	}

	for (int i = 0; i < size; i++) {
		bodies[i].position[0] = (rand() / (double)(RAND_MAX)) * 2 - 1;
		bodies[i].position[1] = (rand() / (double)(RAND_MAX)) * 2 - 1;
		bodies[i].position[2] = (rand() / (double)(RAND_MAX)) * 2 - 1;

		bodies[i].old_position[0] = bodies[i].position[0];
		bodies[i].old_position[1] = bodies[i].position[1];
		bodies[i].old_position[2] = bodies[i].position[2];

		bodies[i].velocity[0] = 0.0;
		bodies[i].velocity[1] = 0.0;
		bodies[i].velocity[2] = 0.0;

		bodies[i].mass = 1.0 / size;
	}

	*ret = bodies;
}

void read_input(struct Body **ret, int *N, const char *file_name)
{
	struct Body *bodies;
	char buffer[1024];
	FILE *file = NULL;

	if ((file = fopen(file_name, "r")) != NULL) {
		if (fgets(buffer, 1024, file) != NULL) {
			*N = atoi(buffer);
		}
		else {
			fprintf(stderr, "End of file!\n");
			exit(1);
		}

		bodies = (struct Body*)calloc(sizeof(struct Body), (*N));
		if (bodies == NULL) {
			fprintf(stderr, "Memory error: %s\n", strerror(errno));
			exit(1);
		}

		for (int i = 0; i < *N; i++) {
			if (fgets(buffer, 1024, file) != NULL) {
				char *token = strtok(buffer, ",");

				bodies[i].position[0] = atof(token);
				token = strtok(NULL, ",");
				bodies[i].position[1] = atof(token);
				token = strtok(NULL, ",");
				bodies[i].position[2] = atof(token);
				token = strtok(NULL, ",");

				bodies[i].old_position[0] = bodies[i].position[0];
				bodies[i].old_position[1] = bodies[i].position[1];
				bodies[i].old_position[2] = bodies[i].position[2];

				bodies[i].velocity[0] = atof(token);
				token = strtok(NULL, ",");
				bodies[i].velocity[1] = atof(token);
				token = strtok(NULL, ",");
				bodies[i].velocity[2] = atof(token);
				token = strtok(NULL, ",");

				bodies[i].mass = atof(token);
			}
			else {
				fprintf(stderr, "End of file!\n");
				exit(1);
			}
		}

		fclose(file);
		*ret = bodies;
	}
}
