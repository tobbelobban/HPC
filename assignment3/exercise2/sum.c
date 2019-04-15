#include "sum.h"

void omp_sum(double *sum_ret)
{
	int i;	
	double sum = 0.0;
	#pragma omp parallel for
	for(i = 0; i < size; ++i) {
		sum += x[i];
	}
	*sum_ret = sum;
}
	
void omp_critical_sum(double *sum_ret)
{
	int i;
	double sum = 0.0;
	#pragma omp parallel for
	for(i = 0; i < size; ++i) {
		#pragma omp critical
		sum += x[i];
	}
	*sum_ret = sum;
}

void omp_atomic_sum(double *sum_ret)
{
	int i;
	double sum = 0.0;	
	#pragma omp parallel for
	for(i = 0; i < size; ++i) {
		#pragma omp atomic
		sum += x[i];
	}
	*sum_ret = sum;
}

void omp_local_sum(double *sum_ret)
{
	const int MAX_THREADS = omp_get_max_threads();
	double partial_sums[MAX_THREADS];
	int i, tid;

	#pragma omp parallel private(tid)
	{
		tid = omp_get_thread_num();
		partial_sums[tid] = 0.0;
		#pragma omp for
		for(i = 0; i < size; ++i) {
			partial_sums[tid] += x[i];
		}
		
	}
	*sum_ret = 0.0;
	for(i = 0; i < MAX_THREADS; ++i) {
		*sum_ret += partial_sums[i];
	}
}

void omp_padded_sum(double *sum_ret)
{
	const int MAX_THREADS = omp_get_max_threads();
	int pad_factor = 64/sizeof(double)*2;
	double partial_sums[MAX_THREADS*pad_factor];
	int i, tid;
	
	#pragma omp parallel private(tid)
	{
		tid = omp_get_thread_num();
		partial_sums[tid*pad_factor] = 0.0;
		#pragma omp for
		for(i = 0; i < size; ++i) {
			partial_sums[tid*pad_factor] += x[i];
		}
	}
	
	*sum_ret = 0.0;
	for(i = 0; i < MAX_THREADS; ++i) {
		*sum_ret += partial_sums[i*pad_factor];
	}
}

void omp_private_sum(double *sum_ret)
{
	int i;
	double t_sum = 0.0;
	double sum = 0.0;
	#pragma omp parallel firstprivate(t_sum) 
	{
		#pragma omp for
		for(i = 0; i < size; ++i) {
			t_sum += x[i];
		}
		#pragma omp atomic
		sum += t_sum;
	}
	*sum_ret = sum;
}

void omp_reduction_sum(double *sum_ret)
{
	int i;
	double sum = 0.0;
	#pragma omp parallel for reduction (+:sum)
	for(i = 0; i < size; ++i) {
		sum += x[i];
	}		
	*sum_ret = sum;
}

