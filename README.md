# HPC
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
