// Cepstrum
#ifndef __HFFCC_CPP_H__
#define __HFFCC_CPP_H__
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <STRING.H>
#include "time.h"
#include "walsh.h"

#define FFT_DATA_TYPE double
//#define FFT_DATA_TYPE float
#define FRAME_SAMPLENUM_MAX  2304 

#define FE_CEP_ORDER           10 /* Number of cepstral coefficients is CEPORDER+1 including c0 */
#define FE_CEP_ORDER_ADD1      (10+1)

#define FE_DELTA                5 /* Order of delta coefficients */
#define FE_DDELTA               3 /* Order of acceleration (delta-delta) coefficients */



#define M_PI   3.14159265358979323846
#define NR_NUM_CHANNELS             23
#define NR_FL                       17


//int preemphasize(float *sample, int sampleN);
//int preprocessing(short *sample, int sampleN, float *out);
/*int Do_hamm_window(float *inputA, int inputN);*/
int Do_hamm_window(double *inputA, int inputN);
int hffcoef(double* frameA, int frameSize, double *hffc_cep, int ceporder,int *frequences);
float LogE(float x);
void FFT_cpp(float *a, float *b, int m, int n_pts, int iff);
//void FFT_c(float *a, float *b, int m, int n_pts, int iff);
void FFT_c(double *a, double *b, int m, int n_pts, int iff);
#endif