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

int hffcoef(double* frameA, int frameSize, float *hffc_cep, int ceporder, int fft_size,int sampleRate, double *filtMig, uintptr_t ffthandle)
{
	
	int i,j;
	double temp_cep;
	//uintptr_t ffthandle;
	double *sample = (double *) malloc(fft_size*sizeof(double)+100);
	double *fSample = (float *) malloc(2*fft_size*sizeof(double)+100);
	memcpy(sample,frameA,frameSize*sizeof(double));
// 	FFT_DATA_TYPE* a=(FFT_DATA_TYPE*)malloc(fft_size*sizeof(FFT_DATA_TYPE));
// 	FFT_DATA_TYPE* b=(FFT_DATA_TYPE*)malloc(fft_size*sizeof(FFT_DATA_TYPE));
// 	int uiLogFFTSize = (int)(log((double)fft_size)/log((double)2)+0.5);
	
	Do_hamm_window(&sample[0],frameSize);
	for(i=0;i<frameSize;i++)
	{ 
		fSample[2*i] = sample[i]; 
		fSample[2*i +1] = 0.0;
	}
	
	for(i=frameSize;i<fft_size;i++)
	{ 
		fSample[2*i] = 0.0; 
		fSample[2*i +1] = 0.0;
	}


	//ffthandle = fa_fft_init(fft_size);
	fa_fft(ffthandle,fSample);
	//fa_fft_uninit(ffthandle);
	
	for(i=0;i<fft_size;i++)
	{ 
		sample[i] = (double)(fSample[2*i] * fSample[2*i]*1.0 + fSample[2*i+1] * fSample[2*i+1]*1.0); 
	}

	

// 	for(i=0;i<frameSize;i++)
// 	{ 
// 		a[i] = frameA[i]; 
// 		b[i]=0; 
// 	}
// 	for(i=frameSize;i<fft_size;i++) 
// 		a[i] = b[i] = 0; 
// 	FFT_c(&a[0], &b[0], uiLogFFTSize, fft_size, 1);
// 	for(i=0;i<fft_size/2;i++)
// 	{
// 		sample[i] = a[i]*a[i] + b[i]*b[i];		
// 	}


	
	for (i=0; i<ceporder; i++)
	{
		temp_cep = 0;
		for(j=0; j<fft_size/2; j++)
		{
			temp_cep += (*(filtMig +(int)(i*0.5*fft_size +j)))*(sample[j]);
		}
		*(hffc_cep+i) = (float)temp_cep;
	}

// 	free(a);
//  	free(b);
	free(sample);
	free(fSample);
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


void FFT_c(double *a, double *b, int m, int n_pts, int iff)
{
	int	l,lmx,lix,lm,li,j1,j2,ii,jj,nv2,nm1,k,lmxy,n;
	double	scl,s,c,arg,T_a,T_b;
	
	n = 1 << m;
	for( l=1 ; l<=m ; l++ ) {
		lmx = 1 << (m - l) ;
		lix = 2 * lmx ;
		scl = 2 * M_PI *1.0/lix ;
		
		if(lmx < n_pts) lmxy = lmx ;
		else lmxy = n_pts ;
		for( lm = 1 ; lm <= lmxy ; lm++ ) {
			arg=((double)(lm-1)*scl) ;
			c = (double)cos(arg) ;
			s = iff * (double)sin(arg) ;
			
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
			a[l]/=(double)n*1.0;
			b[l]/=(double)n*1.0;
		}
	}
}


