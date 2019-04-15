#include "dft.h"

// DFT/IDFT routine
// idft: 1 direct DFT, -1 inverse IDFT (Inverse DFT)
int DFT(int idft, double* xr, double* xi, double* Xr_o, double* Xi_o, int N)
{
//	int i,n,k;
//	int stop = N*N;
//	#pragma omp parallel for private(k,n)
//	for (i = 0; i < stop; ++i) {
//		k = i / N;
//		n = i % N;
//		Xr_o[k] += xr[n] * cos(n * k * PI2 / N) + idft * xi[n] * sin(n * k * PI2 / N);
//		Xi_o[k] += -idft * xr[n] * sin(n * k * PI2 / N)+ xi[n] * cos(n * k * PI2 / N);
//	}


	int n,k;
	#pragma omp parallel for collapse(2)
	for(k = 0; k < N; ++k) {
		for(n = 0; n < N; ++n) {
			Xr_o[k] += xr[n] * cos(n * k * PI2 / N) + idft * xi[n] * sin(n * k * PI2 / N);
	              	Xi_o[k] += -idft * xr[n] * sin(n * k * PI2 / N)+ xi[n] * cos(n * k * PI2 / N);
		}
	}

//	for (int k = 0 ; k < N; k++) { 	
//		double sum_xro = 0.0, sum_xio = 0.0;
//		#pragma omp parallel for reduction(+:sum_xro,sum_xio)
//		for (int n = 0; n < N; n++) {
//			// Real part of X[k]
//			sum_xro += xr[n] * cos(n * k * PI2 / N) + idft * xi[n] * sin(n * k * PI2 / N);
//			// Imaginary part of X[k]
//			sum_xio += -idft * xr[n] * sin(n * k * PI2 / N) + xi[n] * cos(n * k * PI2 / N);
//		}
//		Xr_o[k] += sum_xro;
//		Xi_o[k] += sum_xio;
//	}
		

	// normalize if you are doing IDFT
	if (idft == -1){
		#pragma omp parallel for
		for (n = 0; n < N; n++) {
			Xr_o[n] /= N;
			Xi_o[n] /= N;
		}
	}
	return 1;
}

