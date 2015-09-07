#include "CvLowDef.h"
	//编解码运行函数指针
	COS_DWORD	__stdcall MP1L2_Dec_codec(COS_HANDLE codech,COS_BYTE* indata,COS_DWORD insize,COS_BYTE *outdata,COS_DWORD *outsize);
	//初始化编解码运行函数指针
    COS_HANDLE	__stdcall MP1L2_Dec_initialize(unsigned long sample_rate,  unsigned long bit_rate,  unsigned long chn);
	//卸载编解码运行函数指针
	COS_VOID	__stdcall MP1L2_Dec_uninitialize(COS_HANDLE codech);
	//取得当前的编解码状态
	//COS_BOOL	MP1L2_Dec_state(COS_HANDLE codech,COS_DWORD *sample,COS_DWORD *bitrae,COS_DWORD *stereo);
//	int __stdcall AQ_FeatureExtraction(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand);
	void PCMNormalize(float* frameA, const short* wavpcm,int len);
	void Stereo2Mono(short* WavBuf, int samplsize);
	COS_DWORD FeatureExtraction_PCMbuf(COS_HANDLE WavbufHeader, short *Wavbuf,int inbufsize, float *outbuffer, COS_DWORD *outsize);
	COS_DWORD __stdcall FeatureExtraction_initialize(unsigned long samplerate, int channels, int chn_process);
	COS_VOID __stdcall FeatureExtraction_uninitialize(COS_HANDLE Header);
