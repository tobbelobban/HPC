#ifndef __SUM_H__
#define __SUM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <omp.h>

extern double *x;
extern int size;

void omp_sum(double*);
void omp_atomic_sum(double*);
void omp_local_sum(double*);
void omp_padded_sum(double*);
void omp_private_sum(double*);
void omp_reduction_sum(double*);

#endif
