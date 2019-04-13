#include <stdio.h>
#include <omp.h>

void print_array(const int * my_array, const int sz) {
	int i;
	for(i = 0; i < sz; ++i)
		printf("array[%d] = %d\n",i,*(my_array+i));
}

int main(int argc, char ** argv) {
	int tid;
	const int num_t = 4;
	int num_v[num_t];
	int i;
	for(i = 0; i < num_t; ++i) num_v[i] = 0;
	print_array(num_v,num_t);
	omp_set_num_threads(num_t);

#pragma omp parallel private(tid)
	{
		tid = omp_get_thread_num();
		num_v[tid] = tid;
	}
	print_array(num_v,num_t);
	return 0;
}
