cc -g -O2 matrix_multiply.c -o matrix_multiply.out

for i in {1..10}
do
    aprun ./matrix_multiply.out >> time_not_opt_1000.txt
done
