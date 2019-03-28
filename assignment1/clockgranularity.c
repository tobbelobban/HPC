#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <stdint.h>
#include <unistd.h>

// Use the preprocessor so we know definitively that these are placed inline
#define RDTSC_START()            \
	__asm__ volatile("CPUID\n\t" \
	                 "RDTSC\n\t" \
	                 "mov %%edx, %0\n\t" \
	                 "mov %%eax, %1\n\t" \
	                 : "=r" (start_hi), "=r" (start_lo) \
	                 :: "%rax", "%rbx", "%rcx", "%rdx");

#define RDTSC_STOP()              \
	__asm__ volatile("RDTSCP\n\t" \
	                 "mov %%edx, %0\n\t" \
	                 "mov %%eax, %1\n\t" \
	                 "CPUID\n\t" \
	                 : "=r" (end_hi), "=r" (end_lo) \
	                 :: "%rax", "%rbx", "%rcx", "%rdx");

uint64_t elapsed(uint32_t start_hi, uint32_t start_lo,
                 uint32_t end_hi,   uint32_t end_lo)
{
	uint64_t start = (((uint64_t)start_hi) << 32) | start_lo;
	uint64_t end   = (((uint64_t)end_hi)   << 32) | end_lo;
	return end-start;
}

double mysecond();

int main(int argc, char *argv[])
{
  int32_t start_hi=0, start_lo=0; 
  uint32_t   end_hi=0,   end_lo=0;

  double *ticks, mindist, maxdist, dist;
  int    i, j, n, maxn;

  uint64_t totalTime = 0;

  if (argc > 0) n = atoi(argv[1]);

  ticks = (double *)malloc(n * sizeof(double));

for (j = 0; j < 2; j++) {
  totalTime = 0;
  for (i=0; i<n; i++) {
    //ticks[i] = mysecond();
     RDTSC_START();
     RDTSC_STOP();
     totalTime += elapsed(start_hi, start_lo, end_hi, end_lo);
     ticks[i] = totalTime;
  }
}

  mindist = 1e10;
  maxdist = 0;
  for (i=1; i<n; i++) {
    dist = ticks[i] - ticks[i-1];
    if (dist > 0) {
      if (dist < mindist) mindist = dist;
      if (dist > maxdist) maxdist = dist;
    }
  }

  printf("# min dist = %.2e, max dist = %.2e, total time = %.2e\n",
	 mindist, maxdist, ticks[n-1]-ticks[0]);
  for (i=1; i<n; i++) {
    printf("%d\t%.2e\n", i, ticks[i] - ticks[i-1]);
  }

  return 0;
}
// function with timer                                                             
double mysecond()
{
  struct timeval tp;
  struct timezone tzp;
  int i;
  i = gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

