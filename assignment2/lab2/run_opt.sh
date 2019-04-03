cc -g -O2 matrix_multiply_opt.c -o matrix_multiply_opt.out
perf stat -e instructions,cycles,L1-dcache-load-misses,L1-dcache-loads,dTLB-load-misses,cache-references,branch-misses,branch-instructions ./matrix_multiply_opt.out
