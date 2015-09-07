// Cepstrum
#define FE_CEP_ORDER           12 /* Number of cepstral coefficients is CEPORDER+1 including c0 */
#define FE_CEP_ORDER_ADD1      13
// Dynamic features
#define FE_DELTA                5 /* Order of delta coefficients */
#define FE_DDELTA               3 /* Order of acceleration (delta-delta) coefficients */

// Filter bank
#define FE_NUM_CHANNELS        23 /* Number of bands in filter bank */
#define FE_FBANK_FLOOR    (-50.0) /* Energy floor for filter bank coefficients */
//#define FE_MIN_ENERGY (1)
//#define FE_MIN_LOG_ENERGY (0)
#define M_PI   3.14159265358979323846
#define NR_NUM_CHANNELS             23
#define NR_FL                       17

//typedef struct { /* mel filter banks */
    //int m_lowX;
    //int m_centerX;
    //int m_highX;
//} MfccMelFB;

//typedef struct {
    //int m_lowX;
    //int m_centerX;
    //int m_highX;
//float m_sumWeight;
//} WfMelFB; /* mel filter bank for noise reduction */


int preemphasize(float *sample, int sampleN);
int preprocessing(short *sample, int sampleN, float *out);
int Do_hamm_window(float *inputA, int inputN);
int mel_cepstrum_basic(short *sample, int frameSize, float *mel_cep, int ceporder, int fft_size,int sampleRate);
//void create_mel_filter_bank(int num_bins, int sample_rate,float **filter_coeff);
void create_mel_filter_bank_24(int num_bins,int fft_size, int sample_rate,float **filter_coeff);
int _mel_cepstrum_basic(float *sample, int frameSize, float *mel_cep, int fborder, int ceporder, int fft_size,int sampleRate);
//void MfccInitMelFilterBanks (float startingFrequency, float samplingRate, int fftLength, int numChannels);
float LogE(float x);
void FFT_cpp(float *a, float *b, int m, int n_pts, int iff);
int _filterbank_basic(float *sample, int frameSize, float *filter_bank, int fborder, int fftSize,int sampleRate,int cep_smooth, int cepFilterLen);
void MfccDCT (float *x, float *dctMatrix, int ceporder, int numChannels, float *mel_cep);



void MfccMelFilterBank(float* output,float *sigFFT, int numChannels,int frameSize,float **filter_coeff);//cpp