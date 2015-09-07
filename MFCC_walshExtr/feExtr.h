#ifndef __feExtr_h__
#define __feExtr_h__

#include "stdio.h"
#include "windows.h"
#include <tchar.h> 
#include "wav\waveformat.h"
#include "codec\mpegaudiodec.h"
#include "codec\codecUI.h"
#include "hffcc_cpp\hffcc_cpp.h"
#include "assert.h"

#include "resample\fa_wavfmt.h"
#include "resample\fa_parseopt.h"
#include "resample\fa_fir.h"
#include "resample\fa_resample.h"

#define MPEG_SAMPLE_IN_FRAME_48k        1152
#define MPEG_SAMPLE_IN_FRAME_44k        1058
#define MPEG_SAMPLE_IN_FRAME_32k		768
#define MPEG_SAMPLE_IN_FRAME_8k         192
#define MPEG_SAMPLE_IN_FRAME_4k         96

#define MPEG_SAMPLE_IN_FRAME_2POW_48k   2048
#define MPEG_SAMPLE_IN_FRAME_2POW_44k   2048
#define MPEG_SAMPLE_IN_FRAME_2POW_32k   1024
#define MPEG_SAMPLE_IN_FRAME_2POW_8k    256
#define MPEG_SAMPLE_IN_FRAME_2POW_4k	128
#define FE_DIV_IN_FRAME             0.5

#define WM_PROCESS_MESSAGE   (WM_USER+102)  //jhy 08-4-10
#define WM_PROCESS_FINISH    (WM_USER+103)   //jhy 08-4-10		

//#define OutNormPCM		

typedef  int (*pcallback)(DWORD);


//#define			FRAMENUM	910
#define         WAVSIZE_48k     46080  ////MPADecodeDLL_05-05-16-v1.2B
#define         WAVSIZE_44k     42320       
#define         WAVSIZE_32k     30720  
#define			WAVSIZE_8k      3840
#define         WAVSIZE_4K      960
#define         INSIZE_mp2      7680   ////MPADecodeDLL_05-05-16-v1.2B

#define         WRITELIMIT         1000

#define  MINFRQ 0
#define  MAXFRQ 2000 

#define STARTF 60
#define ENDF 2000
#define BANDUM 10


extern BOOL WaveOut_Init(LPSTR lpszFileName,HWND hWnd);
extern BOOL WavePlay(char* databuf,int datasize);

COS_HANDLE __stdcall FeatureExtraction_initialize(unsigned long samplerate, int channels, int chn_process);
//COS_DWORD __stdcall FeatureExtraction_Buf_Discard(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize);
COS_DWORD __stdcall FeatureExtraction_Buf_Discard(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize);  //²ÐÓà¶ªÆú
COS_VOID __stdcall FeatureExtraction_uninitialize(COS_HANDLE Header);
double *filterDesign(int fs, int FrameLen, int minfrq,int maxfrq,int fftsize);
#endif