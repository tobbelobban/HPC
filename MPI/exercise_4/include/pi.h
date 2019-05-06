#ifndef __PI_H__
#define __PI_H__
#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
char *filename;
int seed;

void init_pi(int, char*);
void cleanup_pi();
void compute_pi(int, int*, double*);

#endif
