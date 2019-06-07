HOW TO COMPILE

If on Beskow:
  1) swap to gnu environment.
  2) in Makefile, change mpicxx to CC 

then call make


HOW TO RUN<br><br>
Locally:<br>
  mpirun -n #processes -genv OMP_NUM_THREADS=#threads -genv I_MPI_PIN_DOMAIN=omp ./bin/mapreduce.out -r #repeats input.txt output.txt<br>
  
Beskow:<br>
  export OMP_NUM_THREADS=#threads<br>
  aprun -n #processes -d $OMP_NUM_THREADS ./bin/mapreduce.out -r #repeats input.txt output.txt<br>
