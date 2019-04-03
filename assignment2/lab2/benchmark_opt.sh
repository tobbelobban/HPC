cc -g -O2 matrix_multiply_opt.c -o matrix_multiply_opt.out

for i in {1..10}
do
    aprun ./matrix_multiply_opt.out >> time_opt_1000.txt
done
