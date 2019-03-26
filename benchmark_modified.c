#include <stdio.h>
#include <sys/time.h>

double mysecond();

int main(int argc, char **argv){
  int  N,i, j;
  if (argc > 0) N = atoi(argv[1]);
  double t1, t2; // timers                                                         
  double a[N], b[N], c[N]; // arrays  
                                             
  // init arrays                                                                   
  for (i = 0; i < N; i++){
    a[i] = 47.0;
    b[i] = 3.1415;
  }

  // measure performance                                                           
  t1 = mysecond();
  for(i = 0; i < N; i++)
    c[i] = a[i]*b[i];
  t2 = mysecond();

  int k;
  t1 = mysecond();
  for(k = 0; k < 100; ++k) {
    
    for(i=0;i<N;++i){
      c[i]=a[i]*b[i];
    }
  }
  t2 = mysecond();
  fprintf(stderr,"for smart comp %d",c[0]);
  printf("%11.8f\n", (t2 - t1)/100);
  return 0;
}

// function with timer                                                             
double mysecond(){
  struct timeval tp;
  struct timezone tzp;
  int i;

  i = gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

