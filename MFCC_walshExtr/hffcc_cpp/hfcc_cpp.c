#include "hffcc_cpp.h"


#define m_fbOrder 24   //40 //zengqi modified

#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) ) 


void normlize_array(float *fearray)
{
	int k;
	float norm=0.0,sum=0.0;
	for(k=0;k<FE_CEP_ORDER +1;k++)
		sum+=(fearray[k])*(fearray[k]);
	norm=sqrt(sum);
	if(norm==0.0)
	{
		for(k=0;k<FE_CEP_ORDER +1;k++)
			fearray[k]=0.0;
	}
	else
	{
		norm=1/norm;
		for(k=0;k<FE_CEP_ORDER +1;k++)
			fearray[k]=fearray[k]*norm;
	}
}

void wavnomalize(float* arrayone,short* arraytwo,int num)
{
	short maxenergy=abs(arraytwo[0]);
	short cur;
	double maxvolume;
	int i;

	for(i=1;i<num;i++)
	{
		cur = abs(arraytwo[i]);
		if(maxenergy< cur)
            maxenergy = cur;
	}

    if(maxenergy==0)
	{
		for(i=0;i<num;i++)
			arrayone[i]=0.0;
	}
	else
	{
		maxvolume=(double)(1.0/maxenergy);
	    for(i=0;i<num;i++)
            arrayone[i] = (float)(arraytwo[i]*maxvolume);
	}
}


//Filter:H(z) = 1-a*z^(-1)
int preemphasize(float *sample, int sampleN)
{
/* Setting emphFac=0 turns off preemphasis. */
int i;
float emphFac = (float)0.97;

for (i = 1; i<sampleN; i++) 
{
   sample[i] = sample[i] - emphFac * sample[i-1];
}

return(1);
}

int preprocessing(short *sample, int sampleN, float *out)
{
	preemphasize(out, sampleN);
	return 1;
}

int preprocessing2(float *out,int sampleN)
{
	int i;

	preemphasize(out, sampleN);
	return 1;
}

//Apply a hamming window to reduce the discontinuity of the edge
int Do_hamm_window(double *inputA, int inputN)
{
int i;

double temp = (double)(2 * M_PI*1.0 / inputN);
double * hamm_window = (double*)malloc(inputN*sizeof(double));

if (hamm_window == NULL)
{
	i = 0;
}

for (i=0 ; i<inputN ; i++ ) hamm_window[i] = (0.54 - 0.46*cos(temp*i));

for (i=0 ; i<inputN ;i++ ) inputA[i] = hamm_window[i]*inputA[i];

free(hamm_window);
return(1);
}

int hffcoef(double* frameA, int frameSize, float *hffc_cep, int ceporder, int *f_bin)
{
	
	int i,j;
	double temp_cep;
	//uintptr_t ffthandle;
	double *sample = (double *) malloc(frameSize*sizeof(double)+100);
	//double *fSample = (float *) malloc(frameSize*sizeof(double)+100);
	memcpy(sample,frameA,frameSize*sizeof(double));

	Do_hamm_window(&sample[0],frameSize);
	
	fwt(frameSize,sample);
	for (i =0; i<frameSize; i++)
	{
		sample[i] = sample[i]*sample[i];
	}
	for(i=0; i<ceporder; i++)
	{
		temp_cep = 0;
		for (j=f_bin[i]; j<=f_bin[i+1]; j++)
		{
			temp_cep = temp_cep+sample[j-1];
		}
		hffc_cep[i] = (float)temp_cep;
	}
	normlize_array(hffc_cep);
	free(sample);
	//free(fSample);
	return 1;
}


float apply_and_sum_filter(int i,float *sample,int frameSize,float filter_coeff[][FRAME_SAMPLENUM_MAX/2]) //cpp
//float apply_and_sum_filter(int i,float *sample,int frameSize,float filter_coeff[][1024])
{
    int n;
    float sum = 0.0;
    for (n=0; n<frameSize/2; n++)
		sum += filter_coeff[i][n] * sample[n];
    return sum;
}

unsigned reverse_bits (unsigned index, unsigned num_bits)
{
    unsigned rev = 0;
	unsigned i;
    for (i=0; i < num_bits; i++)
    {
      rev = (rev << 1) | (index & 1);
      index >>= 1;
    }

    return rev;
}

