cc -g -O2 matrix_multiply.c -o matrix_multiply.out
perf stat -e instructions,cycles,L1-dcache-load-misses,L1-dcache-loads,dTLB-load-misses,cache-references,branch-misses,branch-instructions ./matrix_multiply.out 
1000
