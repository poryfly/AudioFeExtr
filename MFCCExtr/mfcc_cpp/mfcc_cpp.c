#include "mfcc_cpp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#define FFT_DATA_TYPE float
#define FRAME_SAMPLENUM_MAX  2304 
//#define m_sampleRate 48000
//#define real_fft_size    4096
#define m_fbOrder 24   //40 //zengqi modified
//cpp
#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) ) 
//WfMelFB m_MelFB[23];
//float m_MelWeight[real_fft_size/2+1];
//float m_logEnergyFloor;
//float m_energyFloor;

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

for (i = 1; i<sampleN; i++) {
   sample[i] = sample[i] - emphFac * sample[i-1];
}
//sample[0] = (float)(1.0 - emphFac) * sample[0];

return(1);
}

int preprocessing(short *sample, int sampleN, float *out)
{
//wavnomalize(out,sample,sampleN);//cpp
//for(i=0;i<sampleN;i++) 
   //out[i]=sample[i];
//if (m_dither) Dither(out, sampleN);
preemphasize(out, sampleN);
return 1;
}

int preprocessing2(float *out,int sampleN)
{
	int i;

	//for(i=0;i<sampleN;i++) 
	//	out[i]=sample[i];
	preemphasize(out, sampleN);
	return 1;
}

//Apply a hamming window to reduce the discontinuity of the edge
int Do_hamm_window(float *inputA, int inputN)
{
int i;
//FILE *fp1;
//char *outfile1="oneframe.txt";
float temp = (float)(2 * M_PI / (float)inputN);
float* hamm_window = (float*)malloc(inputN*4);
//hamm_window = new float; ////////////////// (inputN*4);
//////////////////////////////////////////////////////////////////////////////////////////////////////

    for (i=0 ; i<inputN ; i++ ) hamm_window[i] = (float)(0.54 - 0.46*cos(temp*i));

for (i=0 ; i<inputN ;i++ ) inputA[i] = hamm_window[i]*inputA[i];
/*if ((fp1=fopen(outfile1,"ab+"))==NULL)
{
	printf("Open Outfile1 Error\n");
}
 fwrite(inputA,sizeof(float),inputN,fp1);
for(i=0;i<10;i++)
	printf("sample2:%f\n",inputA[i]);*/
free(hamm_window);
/////////////////////////////////////////////////////////////////////////////////////////////////////
return(1);
}

int mel_cepstrum_basic(float* frameA, int frameSize, float *mel_cep, int ceporder, int fft_size,int sampleRate)
{
	//float* frameA = (float*)malloc(frameSize*4);
	preprocessing2(&frameA[0],frameSize);
	Do_hamm_window(&frameA[0],frameSize);
	_mel_cepstrum_basic(&frameA[0], frameSize, mel_cep, m_fbOrder, ceporder, fft_size,sampleRate);
	//free(frameA);
	return 1;

}

//cos(PI*i/(24*(j+0.5)))
int MfccInitDCTMatrix (float *dctMatrix, int ceporder, int numChannels)
{
    int i, j;
    for (i = 0; i <= ceporder; i++){//12+1
        for (j = 0; j < numChannels; j++){//23
            dctMatrix[i * numChannels + j] = (float) cos (M_PI * (float) i / (float) numChannels * ((float) j + 0.5));
    if(i==0) dctMatrix[i * numChannels + j]*=(float)sqrt(1/(float)numChannels);
    else     dctMatrix[i * numChannels + j]*=(float)sqrt(2/(float)numChannels);
   }
}
return 1;
}

int _mel_cepstrum_basic(float *sample, int frameSize, float *mel_cep, int fborder, int ceporder, int fft_size,int sampleRate)
{
   float* filter_bank = (float*)malloc(fborder*sizeof(float));
   float* m_dctMatrix=(float*)malloc((FE_CEP_ORDER +1)*m_fbOrder*sizeof(float));
   MfccInitDCTMatrix (&m_dctMatrix[0], ceporder, fborder);
   _filterbank_basic(sample, frameSize, &filter_bank[0], fborder, fft_size,sampleRate,0, 0);
   MfccDCT (filter_bank, &m_dctMatrix[0], ceporder, fborder, mel_cep);
	
   normlize_array(mel_cep);

   //for(i=0;i<10;i++)
	   //printf("mel_cep:%f\n",mel_cep[i]);
   free(m_dctMatrix);
   free(filter_bank);
//   /* scale down to be consistent with other kinds of cepstrum coefficients */
   //f=fborder/(float)fft_size;
   //for(i=0;i<=ceporder;i++) mel_cep[i]*=f;

     return 1;
}

float get_hz_24(int bin, int fft_size, int sample_rate)    //zengqi modified
{
    assert(bin < fft_size);
    if (bin <= fft_size/2)
		//return ((float)sample_rate * ((float)bin /num_bins));	
		return ((float)sample_rate * ((float)bin /fft_size));  
    else // into the negative frequencies
		return -((float)sample_rate * ((float)(fft_size-bin) / fft_size));
}



float get_hz(int bin, int num_bins, int sample_rate) //cpp
{
    assert(bin < num_bins);
    if (bin <= num_bins/2)
	  //return ((float)sample_rate * ((float)bin /num_bins));	
      return ((float)sample_rate * ((float)bin /num_bins));  
    else // into the negative frequencies
      return -((float)sample_rate * ((float)(num_bins-bin) / num_bins));
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

void create_mel_filter_bank_24(int num_bins,int fft_size, int sample_rate,float filter_coeff[][FRAME_SAMPLENUM_MAX/2])  //zengqi modified
{
	short fh = 0.5*sample_rate;
	float maxmelfreq = 1127*log(fh/700.0+1);
	float spacing = (float)maxmelfreq/(m_fbOrder+1); //MEL频率的节距
	int i;
	float freqs[m_fbOrder+2];
	int chan;
	int n;
	float lower,center,upper;
	float height;
	float bin_freq;
	float* filter = (float*)malloc(num_bins/2*sizeof(float));
	
	for(i=0;i<m_fbOrder+2;i++)   //共有m_fbOrder+2个节点，每个滤波器占用3个
		freqs[i]=700*(exp(i*spacing/1127.0)-1);
	for (chan=0; chan<m_fbOrder; chan++)
    {
		lower = freqs[chan];
		center = freqs[chan+1];
		upper = freqs[chan+2];
		
		// Determine the height for a unit-weight filter
		// NB: This works regardless of where the centre is in relation to the width
		height = 2.0 / (upper - lower);
		
        // Create the triangle filter for this channel
        for (n=0; n<num_bins/2; n++)
        {
			bin_freq = get_hz_24(n, fft_size, sample_rate);
			if (bin_freq > lower && bin_freq <= center)
				filter[n] = height * ((bin_freq - lower) / (center-lower));
			else if (bin_freq > center && bin_freq < upper)
				filter[n] = height * ((upper-bin_freq) / (upper-center));
			else
				filter[n] = 0.0;
        }
		
        for(n=0;n<num_bins/2;n++)
			filter_coeff[chan][n]=filter[n];
    }
	free(filter);
	
	
}

void create_mel_filter_bank(int num_bins, int sample_rate,float filter_coeff[][FRAME_SAMPLENUM_MAX/2]) //cpp
{
    //
    // Filter bank parameters
    //
    const float lowest_frequency = 133.3333;
    const unsigned int linear_filters = 13;
    const float linear_spacing = 66.66666666;
    const unsigned int log_filters = 27;
	//const unsigned int log_filters = 20;
    const float log_spacing = 1.0711703;
    unsigned int f;
	int chan;
	int n;
	float lower,center,upper;
	float height;
	float bin_freq;

    // Keep this around for later....
    const unsigned int total_filters = linear_filters + log_filters;
    // Make a filter we can setup and add to the bank
    float* filter = (float*)malloc(num_bins/2*sizeof(float));
   
	
    // Now figure the band edges.  Interesting frequencies are spaced
    // by linear_spacing for a while, then go logarithmic.  First figure
    // all the interesting frequencies.  Lower, center, and upper band
    // edges are all consequtive interesting frequencies.
    //float freqs[42]; // zengqi modified
	float freqs[42];
    for (f=0; f<linear_filters; f++)
        freqs[f] = lowest_frequency + (f * linear_spacing);  
    for (f=0; f<log_filters+2; f++)   
	//for (f=0; f<log_filters; f++)  
      freqs[linear_filters+f] = freqs[linear_filters-1] * pow(log_spacing, (float)(f+1.0));


    // We now want to combine FFT bins so that each filter has unit
    // weight, assuming a triangular weighting function.  First figure
    // out the height of the triangle, then we can figure out each
    // frequencies contribution
    for (chan=0; chan<total_filters; chan++)
    {
    	lower = freqs[chan];
    	center = freqs[chan+1];
    	upper = freqs[chan+2];

    	// Determine the height for a unit-weight filter
    	// NB: This works regardless of where the centre is in relation to the width
    	height = 2.0 / (upper - lower);

        // Create the triangle filter for this channel
        for (n=0; n<num_bins/2; n++)
        {
          bin_freq = get_hz(n, num_bins, sample_rate);
          if (bin_freq > lower && bin_freq <= center)
            filter[n] = height * ((bin_freq - lower) / (center-lower));
          else if (bin_freq > center && bin_freq < upper)
            filter[n] = height * ((upper-bin_freq) / (upper-center));
          else
            filter[n] = 0.0;
        }

        for(n=0;n<num_bins/2;n++)
			filter_coeff[chan][n]=filter[n];
    }
	free(filter);
}

void MfccMelFilterBank(float* output,float *sigFFT, int numChannels,int frameSize,float filter_coeff[][FRAME_SAMPLENUM_MAX/2]) //cpp
//void MfccMelFilterBank(float* output,float *sigFFT, int numChannels,int frameSize,float filter_coeff[][1024])
{
	int n;
	//for(n=0;n<10;n++)
		//printf("sam:%f\n",sigFFT[n]);
	for(n=0;n<numChannels;n++)
		output[n]=apply_and_sum_filter(n,sigFFT,frameSize,filter_coeff);
}





/*(float LogE(float x)
{
if(x>m_energyFloor) return log(x);//ln(x)
else return m_logEnergyFloor;
}*/

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

void FFT_cpp(FFT_DATA_TYPE *a,FFT_DATA_TYPE *b,int m,int n_pts,int iff)
{	
	int n,id;
	unsigned i,j;
	unsigned k;
	unsigned BlockEnd=1;
	unsigned BlockSize;
	double tr,ti;
	double angle_numerator;
	double delta_angle;
	double sm2,sm1,cm2,cm1,w;
	double ar[3], ai[3];
	FFT_DATA_TYPE* c=(FFT_DATA_TYPE*)malloc(m*sizeof(FFT_DATA_TYPE));
    FFT_DATA_TYPE* d=(FFT_DATA_TYPE*)malloc(m*sizeof(FFT_DATA_TYPE));
	for(i=0;i<m;i++)
	{
		c[i]=0;
		d[i]=0;
	}
	//for(i=0;i<10;i++)
	//{
		//printf("a-->%f\n",a[i]);
		//printf("b-->%f\n",b[i]);
	//} no problem
	//printf("a:%f\n",a[3072]);
	for (i=0; i <n_pts; i++ )
    { 
      j=reverse_bits(i, 12);
	  //printf("%d\n",j);
      c[j]=a[i];
	  d[j]=b[i];
    }
	//for(i=0;i<10;i++)
	//{
		//printf("c-->%f\n",c[i]);
		//printf("d-->%f\n",d[i]);
	//}
	for( id=n_pts;id<m;id++)
	{	
      j = reverse_bits (id, 12);
	  c[j] =(double)0;
	  d[j]=(double)0;
	} 
	//for(i=2000;i<2100;i++)
	//{
		//printf("c-->%f\n",c[i]);
		//printf("d-->%f\n",d[i]);
	//} 
	for(BlockSize=2;BlockSize<=m;BlockSize<<=1)
	{	
	  angle_numerator = 2.0 * M_PI;
	  delta_angle = angle_numerator / (double)BlockSize;
      sm2 = sin ( 2.0 * delta_angle );
      sm1 = sin ( delta_angle );
      cm2 = cos ( 2.0 * delta_angle );
      cm1 = cos ( delta_angle );
       w = 2.0 * cm1;
      //for(i=0;i<3;i++)
	  //{
		//ar[i]=(double)0;
		//ai[i]=(double)0;
	  //}
	  for(i=0;i<m;i+=BlockSize)
	  { 
		ar[2] = cm2;
        ar[1] = cm1;

        ai[2] = sm2;
        ai[1] = sm1;
		for(j=i,n=0;n<BlockEnd;j++,n++)
		{ 	    
		  ar[0] = w*ar[1] - ar[2];
          ar[2] = ar[1];
          ar[1] = ar[0];

          ai[0] = w*ai[1] - ai[2];
          ai[2] = ai[1];
          ai[1] = ai[0];
		  k=0;
		  tr=0;
		  ti=0;
		  k = j + BlockEnd;
		  		    
          tr = ar[0]*c[k] - ai[0]*d[k];
		
          ti = ar[0]*d[k] + ai[0]*c[k];
		 //printf("ck:%f\n",c[k]);
          c[k] =c[j] - tr;
		  	//printf("--->ck:%f\n",c[k]);
		  d[k]=d[j]-ti;
		  c[j]+=tr;
		  d[j]+=ti;
		}
	  }
	  BlockEnd=BlockSize;
	}
	for(i=0;i<m;i++)
	{
		a[i]=0;
		b[i]=0;
	}
	for(i=0;i<m;i++)
	{
		a[i]=c[i];
		b[i]=d[i];
	}
	free(c);
	free(d);
}


void FFT_c(float *a, float *b, int m, int n_pts, int iff)
{
	int	l,lmx,lix,lm,li,j1,j2,ii,jj,nv2,nm1,k,lmxy,n;
	float	scl,s,c,arg,T_a,T_b;
	
	n = 1 << m;
	for( l=1 ; l<=m ; l++ ) {
		lmx = 1 << (m - l) ;
		lix = 2 * lmx ;
		scl = 2 * (float)M_PI/(float)lix ;
		
		if(lmx < n_pts) lmxy = lmx ;
		else lmxy = n_pts ;
		for( lm = 1 ; lm <= lmxy ; lm++ ) {
			arg=((float)(lm-1)*scl) ;
			c = (float)cos(arg) ;
			s = iff * (float)sin(arg) ;
			
			for( li = lix ; li <= n ; li += lix ) {
				j1 = li - lix + lm ;
				j2 = j1 + lmx ;
				if(lmxy != n_pts ) {
					T_a=a[j1-1] - a[j2-1] ;
					/* T_a : real part */
					T_b=b[j1-1] - b[j2-1] ;
					/* T_b : imaginary part */
					a[j1-1] = a[j1-1] + a[j2-1] ;
					b[j1-1] = b[j1-1] + b[j2-1] ;
					a[j2-1] = T_a*c + T_b*s ;
					b[j2-1] = T_b*c - T_a*s ;
				} else{
					a[j2-1] = a[j1-1]*c + b[j1-1]*s ;
					b[j2-1] = b[j1-1]*c - a[j1-1]*s ;
				}
			}
		}
	}
	nv2 = n/2 ;
	nm1 = n - 1 ;
	jj = 1 ;
	
	for( ii = 1 ; ii <= nm1 ;) {
		if( ii < jj ) {
			T_a = a[jj-1] ;
			T_b = b[jj-1] ;
			a[jj-1] = a[ii-1] ;
			b[jj-1] = b[ii-1] ;
			a[ii-1] = T_a ;
			b[ii-1] = T_b ;
		}
		k = nv2 ;
		while( k < jj ) {
			jj = jj - k ;
			k = k/2 ;
		}
		jj = jj + k ;
		ii = ii + 1 ;
	}
	if(iff == (-1)){
		for( l=0 ; l<n ; l++ ) {
			a[l]/=(float)n;
			b[l]/=(float)n;
		}
	}
}

int _filterbank_basic(float *sample, int frameSize, float *filter_bank, int fborder, int fftSize,int sampleRate,int cep_smooth, int cepFilterLen)
{

int i;
FFT_DATA_TYPE* a=(FFT_DATA_TYPE*)malloc(fftSize*sizeof(FFT_DATA_TYPE));
FFT_DATA_TYPE* b=(FFT_DATA_TYPE*)malloc(fftSize*sizeof(FFT_DATA_TYPE));
int uiLogFFTSize = (int)(log((double)fftSize)/log((double)2)+0.5);

float filter_coeff[m_fbOrder][FRAME_SAMPLENUM_MAX/2]; //cpp
//float filter_coeff[m_fbOrder][1024];
//create_mel_filter_bank(frameSize,sampleRate,filter_coeff);
create_mel_filter_bank_24(frameSize,fftSize,sampleRate,filter_coeff);
for(i=0;i<frameSize;i++){ a[i] = sample[i]; b[i]=0; }
for(i=frameSize;i<fftSize;i++) a[i] = b[i] = 0;
//for(i=0;i<10;i++)
	//printf("sample2:%f\n",a[i]);
//FFT_cpp( &a[0], &b[0], fftSize, frameSize, 1); 
FFT_c( &a[0], &b[0], uiLogFFTSize, frameSize, 1);
//for(i=0;i<10;i++)
//{
	//printf("a:%lf\n",a[i]);
	//printf("b:%lf\n",b[i]);
//}
for(i=0;i<fftSize;i++){
   a[i] = sqrt(a[i]*a[i] + b[i]*b[i]);
   //b[i] = 0.0;
}
//printf("fftSize:%d\n",fftSize);
//for(i=2045;i<2052;i++)
	//printf("sample3:%f\n",a[i]);

//MfccMelFilterBank (&a[0], fborder, &filter_bank[0], 1); 
MfccMelFilterBank(&filter_bank[0],&a[0],m_fbOrder,frameSize,filter_coeff);
//for(i=0;i<10;i++)
	//printf("sample4:%f\n",filter_bank[i]);
//for (i = 0; i < fborder; i++)  //zengqi modified
//{
	//if(filter_bank[i]==0.0)
		//filter_bank[i]=-200;
	//else
		//filter_bank[i]=log10(filter_bank[i]);
//}
for (i = 0; i < fborder; i++) 
	filter_bank[i]=log10(filter_bank[i]+1);
//for(i=0;i<10;i++)
	//printf("sample4:%f\n",filter_bank[i]);
free(a);
free(b);

return(1);
}

void MfccDCT (float *x, float *dctMatrix, int ceporder, int numChannels, float *mel_cep)
{
    int i, j;
    for (i = 0; i <= ceporder; i++) {
        mel_cep[i] = 0.0;
        for (j = 0; j < numChannels; j++){
            mel_cep[i] += x[j] * dctMatrix[i * numChannels + j];
   }
    } 
    return;
}


