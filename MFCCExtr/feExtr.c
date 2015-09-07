#include "stdio.h"
#include "windows.h"
#include <tchar.h> 
#include "wav\waveformat.h"
#include "codec\mpegaudiodec.h"
#include "codec\codecUI.h"
#include "mfcc_cpp\mfcc_cpp.h"
#include "assert.h"
#include <FCNTL.H>

#define SETMUTE 1
#define FRAME_POWER		1

#define MPEG_SAMPLE_IN_FRAME_48k        1152
#define MPEG_SAMPLE_IN_FRAME_44k        1058
#define MPEG_SAMPLE_IN_FRAME_32k		768
#define MPEG_SAMPLE_IN_FRAME_8k         192

#define MPEG_SAMPLE_IN_FRAME_2POW_48k   2048
#define MPEG_SAMPLE_IN_FRAME_2POW_44k   2048
#define MPEG_SAMPLE_IN_FRAME_2POW_32k   1024
#define MPEG_SAMPLE_IN_FRAME_2POW_8k    256
#define FE_DIV_IN_FRAME             0.5
#define MUTE_THR_PERSAMPLE              600//300//150//1000
#define SKIPLEN					10 * 1024 * 1024

#define WM_PROCESS_MESSAGE   (WM_USER+102)  //jhy 08-4-10
#define WM_PROCESS_FINISH    (WM_USER+103)   //jhy 08-4-10		

//#define OutNormPCM		

typedef  int (*pcallback)(DWORD);

//#define			FRAMENUM	910
#define         WAVSIZE_48k     460800  ////MPADecodeDLL_05-05-16-v1.2B
#define         WAVSIZE_44k     42320       
#define         WAVSIZE_32k     30720  
#define			WAVSIZE_8k      3840

#define         INSIZE_mp2      76800   ////MPADecodeDLL_05-05-16-v1.2B

#define         WRITELIMIT         12500 //1260  150秒

extern BOOL WaveOut_Init(LPSTR lpszFileName,HWND hWnd);
extern BOOL WavePlay(char* databuf,int datasize);

int JudgeMute(short *a,int SamplePoint)
{
	
	float sum = 0.0, AverageValue = 0.0;
	int i;
	for(i = 0; i < SamplePoint; ++i)
		sum += abs(a[i]);
//	if(sum==0)
//		sum=sum;
	AverageValue = sum / SamplePoint;
	
	if(AverageValue > MUTE_THR_PERSAMPLE)
		return 0;
	else
		return 1;
}

void SetMute(short *a,int SamplePoint)
{
	int i;
	for(i = 0; i < SamplePoint; ++i)
	{
		a[i] = abs(a[i]) >= MUTE_THR_PERSAMPLE ? a[i] : 0;
	}
}

void PCMNormalize(float* frameA, const short* wavpcm,int len)
{
	int i;
	short maxpcm=abs(wavpcm[0]);
	short cur;
	double cep_max;
	for(i=1;i<len;i++)
	{
		cur = abs(wavpcm[i]);
		if(maxpcm < cur)
            maxpcm =  cur;
	}

	if(maxpcm==0)
	{
		for(i=0;i<len;i++)
        frameA[i] = 0;
	}
	else
	{
		cep_max = 1.0/maxpcm;
		for(i=0;i<len;i++)
		{
			frameA[i] =(float)((double)wavpcm[i]*cep_max);
		}
	}
	return;
}

unsigned long getfilesize(LPTSTR SourceFile)
{
	int fp;
	char name[1000];
	WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,SourceFile, 256,name, 256, NULL, NULL );
	fp=_open(name,_O_RDONLY);
	if(fp==-1) 
		return -1;
	return _filelength(fp);
}

BOOL CheckMessageQueue() 
{
	MSG msg; 
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{ 
		if(msg.message==WM_QUIT) 
			return FALSE; 
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	} 
	return TRUE; 
}

COS_HANDLE __stdcall FeatureExtraction_initialize(unsigned long samplerate, int channels, int chn_process)
{
	WavbufData *WavBufData = malloc(sizeof(WavbufData));
	WavBufData->samplerate = samplerate;
	WavBufData->channel = channels;
	if (channels == 1)
	{
		chn_process = 1;
	}
	if(samplerate == 48000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_48k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_48k*channels/chn_process;		
	}
	else if (samplerate == 44100)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_44k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_44k*channels/chn_process;
	}
	if(samplerate == 32000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_32k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_32k*channels/chn_process;		
	}
	else if (samplerate == 8000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_8k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_8k*channels/chn_process;
	}
	WavBufData->step =	(int)(WavBufData->BufNum * FE_DIV_IN_FRAME);	
 	WavBufData->processtimes = 0;
	WavBufData->LeftSize = 0;
 	return (COS_HANDLE)WavBufData;
}
COS_VOID __stdcall FeatureExtraction_uninitialize(COS_HANDLE Header)
{
	WavbufData *WavBufData;
	WavBufData = (WavbufData *)Header;
	free(WavBufData);
}

void Stereo2Mono(short* WavBuf, int samplsize)
{
	int i;
	int samplsizehalf = samplsize/2;
	for (i=0;i<samplsizehalf;i++)
	{
		WavBuf[i]=(WavBuf[i * 2] + WavBuf[i * 2 + 1]) / 2;
	}
}

COS_DWORD __stdcall FeatureExtraction_Buf_Continuous(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize)
{
	short* Wavbuf = (short*)inBuf;
	int Fesize = sizeof(float);
	int Pcmsize = sizeof(short);
	float frameA[MPEG_SAMPLE_IN_FRAME_48k+1000];
	float *pFe = frameA;
	WavbufData *WavBufData;
	int curleftsize, newsize;
	*outsize = 0;
//	memcpy(inBuf_tmp, inBuf, insize);
//	Wavbuf = (short*)inBuf_tmp;

	WavBufData = (WavbufData *)Handle;

	newsize = insize - WavBufData->LeftSize;
	if (WavBufData->channel == 2)
	{
		Stereo2Mono(Wavbuf + WavBufData->LeftSize, newsize);
		newsize /= 2;
		insize = newsize + WavBufData->LeftSize;
	}

	curleftsize = insize;
	while(curleftsize >= WavBufData->BufNum)
	{
#ifdef SETMUTE

	#ifdef FRAME_POWER
		int re = JudgeMute(Wavbuf,WavBufData->BufNum);
		if (re)
		{
			memset(outBuf, 0, FE_CEP_ORDER_ADD1 * Fesize);
		}
		else
		{
			PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);
			mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);
		}
	#else
		SetMute(Wavbuf, WavBufData->BufNum);
		PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);				
		mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);						
	#endif

#else
		PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);				
		mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);	
#endif
		
		Wavbuf = Wavbuf + WavBufData->step;
		curleftsize -= WavBufData->step;
		*outsize = *outsize + FE_CEP_ORDER_ADD1;
		outBuf = outBuf + FE_CEP_ORDER_ADD1;
		WavBufData->processtimes++;
	}
	memcpy(WavBufData->LeftBuff,Wavbuf,curleftsize * Pcmsize);
	WavBufData->LeftSize = curleftsize;
	return 0;
}

COS_DWORD __stdcall FeatureExtraction_Buf_Discard(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize)  //残余丢弃
{
	char* inBuf_tmp = malloc(insize + 100);
//	short* Wavbuf = (short*)inBuf;
	//float *pFe =malloc(sizeof(float)*2*MPEG_SAMPLE_IN_FRAME_48k*11); 
	short* Wavbuf;
	int Fesize = sizeof(float);
	float *pFe = malloc(insize * Fesize + 100); 
	float *tmpFe=pFe;
	WavbufData *WavBufData;
	int curleftsize;
	*outsize = 0;
	
	WavBufData = (WavbufData *)Handle;
	memcpy(inBuf_tmp, inBuf, insize);

	Wavbuf = (short*)inBuf_tmp;
	if (WavBufData->channel == 2)
	{
		Stereo2Mono(Wavbuf, insize/2); 
		insize /= 2;
	}
	curleftsize = insize / 2;   //curleftsize 采样点数
	while(curleftsize >= WavBufData->BufNum)
	{
		PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);
		Wavbuf += WavBufData->step;
		mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);
		pFe = pFe + WavBufData->step;
		curleftsize -= WavBufData->step;
		*outsize = *outsize + FE_CEP_ORDER_ADD1 * Fesize;
		outBuf += FE_CEP_ORDER_ADD1;
	}
	free(tmpFe);
	free(inBuf_tmp);
	return (insize - curleftsize * 2);
}

int  FeatureExtraction_ForWav_Ad(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	waveFormat fmt;
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	COS_HANDLE WavbufHeader;
	unsigned int m_process=0;  //jhy 08-4-10
	//short       WavBuf[WAVSIZE_48k];
	short *WavBuf;
	int sizeofshort = sizeof(short);
	//int CurLeftPcmNum = 0;
	int CurTotalPcmNum = 0;
	int NewSampleNum = 0;
	int Fesize = sizeof(float);
	int FeProcessTimes = 0;
   // float T_mel_cep[13*20];
	float *T_mel_cep;
	int  MfccSize;
	int  tmp;
	int first = 1;
	/*int chn_process = 2;*/

	FILE  * decodefile;
	FILE  * sourcefile;
	
	BOOL BProcess;
	BYTE	*pout;	
	int  outsize=0;
//	int  insize=0;
	long totalread=0;
	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread;//=WAVSIZE_48k/2;
	DWORD UnitFrame;
	DWORD HalfFrame;
	short *pfix;
	short *pBackup;
	int chn_process = 2;
	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;
	if(!m_SourceFile||!m_DecodedFile)
		return -1;
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}

	fmt = readWaveHeader(sourcefile);


	if (fmt.frequency == 48000)
	{
		wanttoread =WAVSIZE_48k/2; 
		UnitFrame = MPEG_SAMPLE_IN_FRAME_48k;
	}
	else if (fmt.frequency == 44100)
	{
		wanttoread = WAVSIZE_44k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_44k;
	}
	else if (fmt.frequency == 32000)
	{
		wanttoread = WAVSIZE_32k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_32k;
	}
	else if (fmt.frequency == 8000)
	{
		wanttoread = WAVSIZE_8k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_8k;
	}
	else
		return -10;

	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile)-44;
	fseek(sourcefile,44,SEEK_SET);
	
	m_parentwinhand=(HWND)parentwinhand;

	if (chn_process == 2)
	{
		if(fmt.channels == 1)
			chn_process = 1;
	}
	if (fmt.channels == 1)
		wanttoread = wanttoread/2; 
	
	//UnitFrame = wanttoread / 10; 
	HalfFrame = UnitFrame / 2;
	WavBuf = (short*)malloc(WAVSIZE_48k * sizeofshort);
	T_mel_cep = (float*)malloc(2000 * FE_CEP_ORDER_ADD1 * sizeof(float));
	pBackup = WavBuf;
	WavbufHeader = FeatureExtraction_initialize(fmt.frequency,fmt.channels, chn_process);
	((WavbufData*)WavbufHeader)->LeftSize = 0;
	while(!feof(sourcefile))
	{
		unsigned int real_read;
		pout =(unsigned char *)(WavBuf+((WavbufData*)WavbufHeader)->LeftSize);

        //wanttoread = WAVSIZE/2;
		BProcess=CheckMessageQueue();
		real_read = fread(pout, sizeofshort, wanttoread, sourcefile);
		
		if (first)
		{
			pfix = WavBuf + real_read - UnitFrame;
			while (JudgeMute(WavBuf, UnitFrame) && WavBuf <= pfix)
			{
				WavBuf += HalfFrame;
				real_read -= HalfFrame;
			}
			//first = 0;
		}
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread * sizeofshort;		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
	
		if(real_read >= UnitFrame)
		{
			NewSampleNum = real_read;//新产生的采样点个数
			CurTotalPcmNum = NewSampleNum + ((WavbufData*)WavbufHeader)->LeftSize;
			tmp = NewSampleNum / ((WavbufData*)WavbufHeader)->channel + ((WavbufData*)WavbufHeader)->LeftSize;
			
			FeatureExtraction_Buf_Continuous(WavbufHeader,(char*)WavBuf, CurTotalPcmNum,T_mel_cep,&MfccSize);
			memcpy(WavBuf,WavBuf + tmp - ((WavbufData*)WavbufHeader)->LeftSize, ((WavbufData*)WavbufHeader)->LeftSize*sizeof(short));
			fwrite(T_mel_cep,Fesize,MfccSize,decodefile);
			first = 0;
		}
		else if (first)
		{
			WavBuf = (short*)pout;
			first = 0;
			continue;
		}
 		else
 			break;
	}	
	free(pBackup);
	free(T_mel_cep);
	fclose(sourcefile);
    fclose(decodefile);
	FeatureExtraction_uninitialize(WavbufHeader);
	m_bjobdone=TRUE;	
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);
	return 0;
}

int  FeatureExtraction_ForWav(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	waveFormat fmt;
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	COS_HANDLE WavbufHeader;
	unsigned int m_process=0;  //jhy 08-4-10
	//short       WavBuf[WAVSIZE_48k];
	short *WavBuf;
	int sizeofshort = sizeof(short);
	//int CurLeftPcmNum = 0;
	int CurTotalPcmNum = 0;
	int NewSampleNum = 0;
	int Fesize = sizeof(float);
	int FeProcessTimes = 0;
    //float T_mel_cep[13*20];
	float *T_mel_cep;
	int  MfccSize;
	int  tmp;
	int UnitFrame;
		//int mlgb = 0;
	/*int chn_process = 2;*/

	FILE  * decodefile;
	FILE  * sourcefile;
	
	BOOL BProcess;
	BYTE	*pout;	
	int  outsize=0;
//	int  insize=0;
	long totalread=0;
	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread;//=WAVSIZE_48k/2;
	int chn_process = 2;
	int framecount = 0;
	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;
	
	
	if(!m_SourceFile||!m_DecodedFile)
		return -1;
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
	fmt = readWaveHeader(sourcefile);
	if (fmt.frequency == 48000)
	{
		wanttoread =WAVSIZE_48k/2; 
		UnitFrame = MPEG_SAMPLE_IN_FRAME_48k;
	}
	else if (fmt.frequency == 44100)
	{
		wanttoread = WAVSIZE_44k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_44k;
	}
	else if (fmt.frequency == 32000)
	{
		wanttoread = WAVSIZE_32k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_32k;
	}
	else if (fmt.frequency == 8000)
	{
		wanttoread = WAVSIZE_8k/2;
		UnitFrame = MPEG_SAMPLE_IN_FRAME_8k;
	}
	else
		return -10;

	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile)-44;
	fseek(sourcefile,44,SEEK_SET);
	
	m_parentwinhand=(HWND)parentwinhand;

	if (chn_process == 2)
	{
		if(fmt.channels == 1)
			chn_process = 1;
	}
	if (fmt.channels == 1)
		wanttoread = wanttoread/2; 
		
	WavBuf = (short*)malloc(WAVSIZE_48k * sizeofshort);
	T_mel_cep = (float*)malloc(2000 * FE_CEP_ORDER_ADD1 * sizeof(float));
	WavbufHeader = FeatureExtraction_initialize(fmt.frequency,fmt.channels, chn_process);
	((WavbufData*)WavbufHeader)->LeftSize = 0;
	while(!feof(sourcefile))
	{
		unsigned int real_read;
		pout =(unsigned char *)(WavBuf+((WavbufData*)WavbufHeader)->LeftSize);
        //wanttoread = WAVSIZE/2;
		BProcess=CheckMessageQueue();
		real_read = fread(pout,sizeofshort,wanttoread,sourcefile);		
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread * sizeofshort;		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		if(real_read > UnitFrame)
		{
			NewSampleNum = real_read;//新产生的采样点个数
			CurTotalPcmNum = NewSampleNum + ((WavbufData*)WavbufHeader)->LeftSize;
			tmp = NewSampleNum / ((WavbufData*)WavbufHeader)->channel + ((WavbufData*)WavbufHeader)->LeftSize;

			FeatureExtraction_Buf_Continuous(WavbufHeader,(char*)WavBuf, CurTotalPcmNum,T_mel_cep,&MfccSize);
			memcpy(WavBuf,WavBuf + tmp - ((WavbufData*)WavbufHeader)->LeftSize, ((WavbufData*)WavbufHeader)->LeftSize*sizeof(short));
			fwrite(T_mel_cep,Fesize,MfccSize,decodefile);
		}
 		else
 			break;
	}	
	free(WavBuf);
	free(T_mel_cep);
	fclose(sourcefile);
    fclose(decodefile);
	FeatureExtraction_uninitialize(WavbufHeader);
	m_bjobdone=TRUE;	
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);
	return 0;
}

int __stdcall FeatureExtraction_ForPCM(LPTSTR m_SourceFile,LPTSTR m_DecodedFile, int PcmSamplerate, int Channels, long *parentwinhand)
{
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	COS_HANDLE WavbufHeader;
	unsigned int m_process=0;  //jhy 08-4-10
	short       WavBuf[WAVSIZE_48k];
	int sizeofshort = sizeof(short);
	//int CurLeftPcmNum = 0;
	int CurTotalPcmNum = 0;
	int NewSampleNum = 0;
	int Fesize = sizeof(float);
	int FeProcessTimes = 0;
    float T_mel_cep[13*20];
	int  MfccSize;
	int tmp;

	/*int chn_process = 2;*/

	FILE  * decodefile;
	FILE  * sourcefile;
	
	BOOL BProcess;
	BYTE	*pout;	
	int  outsize=0;
//	int  insize=0;
	long totalread=0;
	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread;//=WAVSIZE_48k/2;
	int chn_process = 2;
	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;
	
	if(!m_SourceFile||!m_DecodedFile)
		return -1;
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}

	if (PcmSamplerate == 48000)
	{
		wanttoread =WAVSIZE_48k/2; 
	}
	else if (PcmSamplerate == 44100)
	{
		wanttoread = WAVSIZE_44k/2;
	}
	else if (PcmSamplerate == 32000)
	{
		wanttoread = WAVSIZE_32k/2;
	}
	else if (PcmSamplerate == 8000)
	{
		wanttoread = WAVSIZE_8k/2;
	}
	else
		return -10;
	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile);
	fseek(sourcefile,0,SEEK_SET);
	
	m_parentwinhand=(HWND)parentwinhand;

	if (chn_process == 2)
	{
		if(Channels == 1)
			chn_process = 1;
	}
	if (Channels == 1)
		wanttoread = wanttoread/2; 
		

	WavbufHeader = FeatureExtraction_initialize(PcmSamplerate, Channels, chn_process);
	((WavbufData*)WavbufHeader)->LeftSize = 0;
	while(!feof(sourcefile))
	{
		unsigned int real_read;
		pout =(unsigned char *)(WavBuf+((WavbufData*)WavbufHeader)->LeftSize);
        //wanttoread = WAVSIZE/2;
		BProcess=CheckMessageQueue();
		real_read = fread(pout,sizeofshort,wanttoread,sourcefile);		
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread * sizeofshort;		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		//if(real_read >0)
		if(real_read > wanttoread/10)
		{
			NewSampleNum = real_read;//新产生的采样点个数
			CurTotalPcmNum = NewSampleNum + ((WavbufData*)WavbufHeader)->LeftSize;//当前剩余未处理的采样点的个数
	
			tmp = NewSampleNum/((WavbufData*)WavbufHeader)->channel + ((WavbufData*)WavbufHeader)->LeftSize;
			FeatureExtraction_Buf_Continuous(WavbufHeader,(char*)WavBuf, CurTotalPcmNum,T_mel_cep,&MfccSize);
			memcpy(WavBuf,WavBuf + tmp - ((WavbufData*)WavbufHeader)->LeftSize ,((WavbufData*)WavbufHeader)->LeftSize*sizeof(short));
			fwrite(T_mel_cep,Fesize,MfccSize,decodefile);
		}
 		else
 			break;
	}

	fclose(sourcefile);
    fclose(decodefile);
	FeatureExtraction_uninitialize(WavbufHeader);
	m_bjobdone=TRUE;	

	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);
	return 0;
}

int __stdcall FeatureExtraction_ForS48_setMute(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	unsigned int m_process=0;  //jhy 08-4-10
	short       WavBuf[WAVSIZE_48k];
	BYTE       	MpaBuf[INSIZE_mp2];
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;
   // int CurLeftWav_short = 0;
	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe;
	short* pWav;
	int FeStep;
	int FeProcessTimes = 0;
	float *MFCC, *pMFCC;
	float frameA[2*MPEG_SAMPLE_IN_FRAME_48k*11]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile;
	FILE  * sourcefile;
	BOOL BProcess;
	BYTE	*pin;
	BYTE	*pout;

	int  outsize=0;
	int  insize=0;
	long totalread=0;
	int memlen = 0;
	//unsigned long baktime=0,curtime=0,tcount=0;
    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
//	BYTE* cur=WavBuf;

//    waveFormat fmt;

	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
	if(!m_SourceFile||!m_DecodedFile)
		return -1;

	//////////////
   // WaveOut_Init("c:\\audio\\5.wav",NULL);
   ///////////////
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#ifdef OutNormPCM
	if ((datafile = _wfopen(m_Data, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#endif
  //////////////////////////timecounter///////////////////////////////////////
 // baktime=curtime=GetTickCount();
  //////////////////////////timecounter///////////////////////////////////////

	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile);
	fseek(sourcefile,0,SEEK_SET);

	//MFCC = (float*)malloc(Fesize * FE_CEP_ORDER_ADD1 * (m_ltotalfilelen * 2 / 768 - 1) + 1000);
	memlen = Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024;
	MFCC = (float *)malloc(memlen);
	memset(MFCC, 0, memlen);
	pMFCC = MFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;
        BProcess=CheckMessageQueue();
		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MFCC);
				fclose(sourcefile);
				fclose(decodefile);
				MP1L2_Dec_uninitialize(codech);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				//pout=WAVSIZE+WavBuf;
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
				
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			while ( CurLeftPcmNum>= FeDataNum )
			{
#ifdef FRAME_POWER
				int re = JudgeMute(pWav, FeDataNum);
				if (re)
				{
					memset(pMFCC, 0, FE_CEP_ORDER_ADD1 * Fesize);
				}
				else
				{
					PCMNormalize(pFe,pWav,FeDataNum);
					mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);
				}
#else
				SetMute(pWav, FeDataNum);
				PCMNormalize(pFe,pWav,FeDataNum);				
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);							
#endif
			

#ifdef OutNormPCM
				fwrite(pFe,sizeof(float),FeDataNum,datafile);
#endif		   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;
			//mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum);//for mfcc_c
			//fwrite(T_mel_cep,Fesize,FE_CEP_ORDER_ADD1,decodefile);
				pWav = pWav + FeStep;//前移半帧，立体声=1152
			//	pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			assert(CurLeftPcmNum==1152/chn_process);
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
//			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		}
		//continue the leftbit in current running and left ones in files

	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

		if (countFrame >= WRITELIMIT)
		{
/********************************1000帧写一次fe**************************************/
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);

			countFrame = 0;
			pMFCC = MFCC;
		}

	} 
	
	if (countFrame > 0)
	{
		fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
	}

	
#ifdef OutNormPCM	
	fclose(datafile);
#endif
	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}

int __stdcall FeatureExtraction_ForS48(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	unsigned int m_process=0;  //jhy 08-4-10
	short       WavBuf[WAVSIZE_48k];
	BYTE       	MpaBuf[INSIZE_mp2];
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;
   // int CurLeftWav_short = 0;
	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe;
	short* pWav;
	int FeStep;
	int FeProcessTimes = 0;
	float *MFCC, *pMFCC;
	float frameA[2*MPEG_SAMPLE_IN_FRAME_48k*11]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile;
	FILE  * sourcefile;
	BOOL BProcess;
	BYTE	*pin;
	BYTE	*pout;

	int  outsize=0;
	int  insize=0;
	long totalread=0;
	int memlen = 0;
	//unsigned long baktime=0,curtime=0,tcount=0;
    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
//	BYTE* cur=WavBuf;

//    waveFormat fmt;

	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
	if(!m_SourceFile||!m_DecodedFile)
		return -1;

	//////////////
   // WaveOut_Init("c:\\audio\\5.wav",NULL);
   ///////////////
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#ifdef OutNormPCM
	if ((datafile = _wfopen(m_Data, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#endif
  //////////////////////////timecounter///////////////////////////////////////
 // baktime=curtime=GetTickCount();
  //////////////////////////timecounter///////////////////////////////////////

	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile);
	fseek(sourcefile,0,SEEK_SET);

	//MFCC = (float*)malloc(Fesize * FE_CEP_ORDER_ADD1 * (m_ltotalfilelen * 2 / 768 - 1) + 1000);
	memlen = Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024;
	MFCC = (float *)malloc(memlen);
	memset(MFCC, 0, memlen);
	pMFCC = MFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;
        BProcess=CheckMessageQueue();
		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MFCC);
				fclose(sourcefile);
				fclose(decodefile);
				MP1L2_Dec_uninitialize(codech);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				//pout=WAVSIZE+WavBuf;
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
				
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			while ( CurLeftPcmNum>= FeDataNum )
			{
				PCMNormalize(pFe,pWav,FeDataNum);
// 				mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);

#ifdef OutNormPCM
				fwrite(pFe,sizeof(float),FeDataNum,datafile);
#endif		   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;
			//mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum);//for mfcc_c
			//fwrite(T_mel_cep,Fesize,FE_CEP_ORDER_ADD1,decodefile);
				pWav = pWav + FeStep;//前移半帧，立体声=1152
				pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			assert(CurLeftPcmNum==1152/chn_process);
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
//			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		}
		//continue the leftbit in current running and left ones in files

	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

		if (countFrame >= WRITELIMIT)
		{
/********************************1000帧写一次fe**************************************/
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);

			countFrame = 0;
			pMFCC = MFCC;
		}

	} 
	
	if (countFrame > 0)
	{
		fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
	}

	
#ifdef OutNormPCM	
	fclose(datafile);
#endif
	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}


int __stdcall FeatureExtraction_ForS48_rearMute(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen;
    unsigned long m_ltotalfilelen;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	unsigned int m_process=0;  //jhy 08-4-10
// 	short       WavBuf[WAVSIZE_48k];
// 	BYTE       	MpaBuf[INSIZE_mp2];
 	short *WavBuf = (short*)malloc(WAVSIZE_48k * sizeof(short));
 	BYTE *MpaBuf = (BYTE *)malloc(INSIZE_mp2 * sizeof(BYTE));
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;
	int totalFrame;
	int volEndFrame;
   // int CurLeftWav_short = 0;
	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe = NULL;
	short* pWav = NULL;
	int FeStep;
	int FeProcessTimes = 0;
	long s48_framenum, fe_framenum, blocknum;
	int writenum = 0;
	float *MFCC, *pMFCC;
	float frameA[MPEG_SAMPLE_IN_FRAME_48k+1000]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile;
	FILE  * sourcefile;
	BOOL BProcess;
	BYTE	*pin = NULL;
	BYTE	*pout = NULL;
	int UnitFrame;
	int  outsize=0;
	int  insize=0;
	long totalread=0;
	int memlen = 0;
	//unsigned long baktime=0,curtime=0,tcount=0;
    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
//	BYTE* cur=WavBuf;

//    waveFormat fmt;

	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
	if(!m_SourceFile||!m_DecodedFile)
		return -1;

	//////////////
   // WaveOut_Init("c:\\audio\\5.wav",NULL);
   ///////////////
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#ifdef OutNormPCM
	if ((datafile = _wfopen(m_Data, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#endif
  //////////////////////////timecounter///////////////////////////////////////
 // baktime=curtime=GetTickCount();
  //////////////////////////timecounter///////////////////////////////////////


	UnitFrame = MPEG_SAMPLE_IN_FRAME_48k;
	m_ltotalfilelen = getfilesize(m_SourceFile);
	fseek(sourcefile, 0, SEEK_SET);
	
	s48_framenum = m_ltotalfilelen / 768;
	fe_framenum = s48_framenum * 2 - 1;
	blocknum = fe_framenum / WRITELIMIT;

	//MFCC = (float*)malloc(Fesize * FE_CEP_ORDER_ADD1 * (m_ltotalfilelen * 2 / 768 - 1) + 1000);//MFCC内存一次开好，不再1000帧写一次文件
	memlen = Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024;
	MFCC = (float *)malloc(memlen * 2);
	memset(MFCC, 0, memlen);

	pMFCC = MFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;
	totalFrame = 0;
	volEndFrame = -1;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;
        BProcess=CheckMessageQueue();
		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MpaBuf);
				free(WavBuf);
				free(MFCC);
				fclose(sourcefile);
				fclose(decodefile);
				MP1L2_Dec_uninitialize(codech);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				//pout=WAVSIZE+WavBuf;
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
				
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			while ( CurLeftPcmNum >= FeDataNum )
			{
				int re = JudgeMute(pWav, UnitFrame);

				if(re && volEndFrame < 0)        //判断是否静音，是静音且前一帧为非静音才打点，否则还原
					volEndFrame = countFrame;

				else if(re == 0)
					volEndFrame = -1;

				PCMNormalize(pFe,pWav,FeDataNum);
// 				mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);

#ifdef OutNormPCM
				fwrite(pFe,sizeof(float),FeDataNum,datafile);
#endif		   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;
				totalFrame++;
			//mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum);//for mfcc_c
			//fwrite(T_mel_cep,Fesize,FE_CEP_ORDER_ADD1,decodefile);
				pWav = pWav + FeStep;//前移半帧，立体声=1152
			//	pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			assert(CurLeftPcmNum==1152/chn_process);
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
//			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		}
		//continue the leftbit in current running and left ones in files


	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

/********************************12500帧写一次fe**************************************/
		if (countFrame >= WRITELIMIT && writenum < blocknum - 1)
		{
	
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
			writenum++;
			countFrame = 0;
			volEndFrame = -1;
			pMFCC = MFCC;
		}

	} 

	if (countFrame > 0)
	{
		if(volEndFrame < 0)
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		else
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * volEndFrame, decodefile);
	}

	
#ifdef OutNormPCM	
	fclose(datafile);
#endif
	free(MpaBuf);
	free(WavBuf);
	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}

int __stdcall FeatureExtraction_ForS48_setMute_rearMute_Log(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen;
    unsigned long m_ltotalfilelen;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	unsigned int m_process=0;  //jhy 08-4-10
// 	short       WavBuf[WAVSIZE_48k];
// 	BYTE       	MpaBuf[INSIZE_mp2];
 	short *WavBuf = (short*)malloc(WAVSIZE_48k * sizeof(short));
 	BYTE *MpaBuf = (BYTE *)malloc(INSIZE_mp2 * sizeof(BYTE));
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;
	int totalFrame;
	int volEndFrame;
   // int CurLeftWav_short = 0;
	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe = NULL;
	short* pWav = NULL;
	int FeStep;
	int FeProcessTimes = 0;
	long s48_framenum, fe_framenum, blocknum;
	int writenum = 0;
	float *MFCC, *pMFCC;
	float frameA[MPEG_SAMPLE_IN_FRAME_48k+1000]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile;
	FILE  * sourcefile;
	BOOL BProcess;
	BYTE	*pin = NULL;
	BYTE	*pout = NULL;
	int UnitFrame;
	int  outsize=0;
	int  insize=0;
	long totalread=0;
	int memlen = 0;
	//unsigned long baktime=0,curtime=0,tcount=0;
    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
//	BYTE* cur=WavBuf;

//    waveFormat fmt;

	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
	if(!m_SourceFile||!m_DecodedFile)
		return -1;

	//////////////
   // WaveOut_Init("c:\\audio\\5.wav",NULL);
   ///////////////
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#ifdef OutNormPCM
	if ((datafile = _wfopen(m_Data, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#endif
  //////////////////////////timecounter///////////////////////////////////////
 // baktime=curtime=GetTickCount();
  //////////////////////////timecounter///////////////////////////////////////


	UnitFrame = MPEG_SAMPLE_IN_FRAME_48k;
	m_ltotalfilelen = getfilesize(m_SourceFile);
	fseek(sourcefile, 0, SEEK_SET);
	
	s48_framenum = m_ltotalfilelen / 768;
	fe_framenum = s48_framenum * 2 - 1;
	blocknum = fe_framenum / WRITELIMIT;

	//MFCC = (float*)malloc(Fesize * FE_CEP_ORDER_ADD1 * (m_ltotalfilelen * 2 / 768 - 1) + 1000);//MFCC内存一次开好，不再1000帧写一次文件
	memlen = Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024;
	MFCC = (float *)malloc(memlen * 2);
	memset(MFCC, 0, memlen);

	pMFCC = MFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;
	totalFrame = 0;
	volEndFrame = -1;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;
        BProcess=CheckMessageQueue();
		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MpaBuf);
				free(WavBuf);
				free(MFCC);
				fclose(sourcefile);
				fclose(decodefile);
				MP1L2_Dec_uninitialize(codech);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				//pout=WAVSIZE+WavBuf;
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
				
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			while ( CurLeftPcmNum >= FeDataNum )
			{
#ifdef SETMUTE
				
	#ifdef FRAME_POWER
				int re = JudgeMute(pWav,UnitFrame);
				if (re)
				{
					memset(pMFCC, 0, FE_CEP_ORDER_ADD1 * Fesize);
					if (volEndFrame < 0)
					{
						volEndFrame = countFrame;		//判断是否静音，是静音且前一帧为非静音才打点，否则还原
					}
				}
				else
				{
					PCMNormalize(pFe,pWav,FeDataNum);
					mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);
					volEndFrame = -1;
				}
	#else
				SetMute(pWav,UnitFrame);
				PCMNormalize(pFe,pWav,FeDataNum);				
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);						
	#endif
				
#else
				PCMNormalize(pFe,pWav,FeDataNum);				
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);						
#endif


#ifdef OutNormPCM
				fwrite(pFe,sizeof(float),FeDataNum,datafile);
#endif		   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;
				totalFrame++;
			//mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum);//for mfcc_c
			//fwrite(T_mel_cep,Fesize,FE_CEP_ORDER_ADD1,decodefile);
				pWav = pWav + FeStep;//前移半帧，立体声=1152
			//	pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			assert(CurLeftPcmNum==1152/chn_process);
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
//			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		}
		//continue the leftbit in current running and left ones in files


	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

/********************************12500帧写一次fe**************************************/
		if (countFrame >= WRITELIMIT && writenum < blocknum - 1)
		{
	
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
			writenum++;
			countFrame = 0;
			volEndFrame = -1;
			pMFCC = MFCC;
		}

	} 

	if (countFrame > 0)
	{
		if(volEndFrame < 0)
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		else
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * volEndFrame, decodefile);
	}

	
#ifdef OutNormPCM	
	fclose(datafile);
#endif
	free(MpaBuf);
	free(WavBuf);
	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}

int __stdcall FeatureExtraction_ForS48_rearMute_Ad(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen;
    unsigned long m_ltotalfilelen;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	unsigned int m_process=0;  //jhy 08-4-10
// 	short       WavBuf[WAVSIZE_48k];
// 	BYTE       	MpaBuf[INSIZE_mp2];
 	short *WavBuf = (short*)malloc(WAVSIZE_48k * sizeof(short));
 	BYTE *MpaBuf = (BYTE *)malloc(INSIZE_mp2 * sizeof(BYTE));
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;
	int totalFrame;
	int volEndFrame;
   // int CurLeftWav_short = 0;
	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe = NULL;
	short* pWav = NULL;
	short *pfix = NULL;
	int FeStep;
	int FeProcessTimes = 0;
	long s48_framenum, fe_framenum, blocknum;
	int writenum = 0;
	float *MFCC, *pMFCC;
	float frameA[MPEG_SAMPLE_IN_FRAME_48k+1000]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile;
	FILE  * sourcefile;
	BOOL BProcess;
	BYTE	*pin = NULL;
	BYTE	*pout = NULL;
	int UnitFrame;
	int  outsize=0;
	int  insize=0;
	long totalread=0;
	int memlen = 0;
	int first = 1;
	//unsigned long baktime=0,curtime=0,tcount=0;
    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
//	BYTE* cur=WavBuf;

//    waveFormat fmt;

	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
	if(!m_SourceFile||!m_DecodedFile)
		return -1;

	//////////////
   // WaveOut_Init("c:\\audio\\5.wav",NULL);
   ///////////////
	if ((sourcefile = _wfopen(m_SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(m_DecodedFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#ifdef OutNormPCM
	if ((datafile = _wfopen(m_Data, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
#endif
  //////////////////////////timecounter///////////////////////////////////////
 // baktime=curtime=GetTickCount();
  //////////////////////////timecounter///////////////////////////////////////


	UnitFrame = MPEG_SAMPLE_IN_FRAME_48k;
	m_ltotalfilelen = getfilesize(m_SourceFile);
	fseek(sourcefile, 0, SEEK_SET);
	
	s48_framenum = m_ltotalfilelen / 768;
	fe_framenum = s48_framenum * 2 - 1;
	blocknum = fe_framenum / WRITELIMIT;

	//MFCC = (float*)malloc(Fesize * FE_CEP_ORDER_ADD1 * (m_ltotalfilelen * 2 / 768 - 1) + 1000);//MFCC内存一次开好，不再1000帧写一次文件
	memlen = Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024;
	MFCC = (float *)malloc(memlen * 2);
	memset(MFCC, 0, memlen);

	pMFCC = MFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;
	totalFrame = 0;
	volEndFrame = -1;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;
        BProcess=CheckMessageQueue();
		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MpaBuf);
				free(WavBuf);
				free(MFCC);
				fclose(sourcefile);
				fclose(decodefile);
				MP1L2_Dec_uninitialize(codech);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				//pout=WAVSIZE+WavBuf;
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
				
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			if (first)
			{
				pfix = pWav + CurLeftPcmNum - UnitFrame;
				while (JudgeMute(pWav, UnitFrame) && pWav <= pfix)
				{
					pWav += UnitFrame;
					CurLeftPcmNum -= UnitFrame;
				}
			}
			if (first && CurLeftPcmNum < FeDataNum)
			{
				pWav = WavBuf;
				first = 0;
				continue;
			}
			while ( CurLeftPcmNum >= FeDataNum )
			{
#ifdef SETMUTE		
	#ifdef FRAME_POWER
				int re = JudgeMute(pWav,UnitFrame);
				if(re && volEndFrame < 0)        //判断是否静音，是静音且前一帧为非静音才打点，否则还原
					volEndFrame = countFrame;

				else if(re == 0)
					volEndFrame = -1;

					PCMNormalize(pFe,pWav,FeDataNum);
					mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);

	#else
				SetMute(pWav,UnitFrame);
				PCMNormalize(pFe,pWav,FeDataNum);				
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);						
	#endif
				
#else
				PCMNormalize(pFe,pWav,FeDataNum);				
				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);						
#endif


#ifdef OutNormPCM
				fwrite(pFe,sizeof(float),FeDataNum,datafile);
#endif		   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;
				totalFrame++;
				first = 0;
			//mel_cepstrum_basic(pFe, FeDataNum, T_mel_cep, FE_CEP_ORDER, FeDataNum);//for mfcc_c
			//fwrite(T_mel_cep,Fesize,FE_CEP_ORDER_ADD1,decodefile);
				pWav = pWav + FeStep;//前移半帧，立体声=1152
			//	pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			assert(CurLeftPcmNum==1152/chn_process);
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
//			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		}
		//continue the leftbit in current running and left ones in files


	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

/********************************12500帧写一次fe**************************************/
		if (countFrame >= WRITELIMIT && writenum < blocknum - 1)
		{
	
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
			writenum++;
			countFrame = 0;
			volEndFrame = -1;
			pMFCC = MFCC;
		}

	} 

	if (countFrame > 0)
	{
		if(volEndFrame < 0)
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
		else
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * volEndFrame, decodefile);
	}

	
#ifdef OutNormPCM	
	fclose(datafile);
#endif
	free(MpaBuf);
	free(WavBuf);
	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}

int __stdcall FeatureExtraction_AdFile(LPTSTR m_SourceFile,LPTSTR m_DecodedFile, long *parentwinhand)
{
	wchar_t* extentions;
	wchar_t Wavexten[4] = {'w','a','v','\0'};
	wchar_t S48xten[4] = {'s','4','8','\0'};
	wchar_t Wavexten2[4] = {'W','A','V','\0'};
	wchar_t Wavexten3[4] = {'W','a','v','\0'};
	wchar_t S48xten2[4] = {'S','4','8','\0'};
	wchar_t SLGexten[4] = {'s','l','g','\0'};
	wchar_t MP2exten[4] = {'m','p','2','\0'};
	wchar_t SLGexten2[4] = {'S','L','G','\0'};
	wchar_t SLGexten3[4] = {'S','l','g','\0'};
	int re;
	char Log[255] = {'\0'};
	__try
	{
		extentions = wcsrchr(m_SourceFile,'.');
		extentions++;
		if (wcscmp(extentions,Wavexten)==0 || wcscmp(extentions,Wavexten2)==0 || wcscmp(extentions,Wavexten3)==0)
		{
			re = FeatureExtraction_ForWav_Ad(m_SourceFile, m_DecodedFile, parentwinhand);
		}
		else if(wcscmp(extentions,S48xten)==0 || wcscmp(extentions,S48xten2)==0 || wcscmp(extentions,SLGexten)==0 || wcscmp(extentions,SLGexten2)==0 || wcscmp(extentions,SLGexten3)==0 || wcscmp(extentions,MP2exten)==0 )
		{
			//	re = FeatureExtraction_ForS48(m_SourceFile, m_DecodedFile, parentwinhand);
			//re =FeatureExtraction_ForS48_setMute(m_SourceFile, m_DecodedFile, parentwinhand);
			re =FeatureExtraction_ForS48_rearMute_Ad(m_SourceFile, m_DecodedFile, parentwinhand);
		}
		else
			re = -10;
		
		return re;
	}
	__except(1)
	{
		sprintf(Log,"%s\n","FeatureExtraction_AdFile try catch error"); 
		//	LogWrite(Log,ERRORTYPE);
		//printf("%s",Log);
		return -9;
		
	}
}

int __stdcall FeatureExtraction_LogFile(LPTSTR m_SourceFile,LPTSTR m_DecodedFile, long *parentwinhand)
{
	wchar_t* extentions;
	char Log[255] = {'\0'};
	wchar_t Wavexten[4] = {'w','a','v','\0'};
	wchar_t S48xten[4] = {'s','4','8','\0'};
	wchar_t Wavexten2[4] = {'W','A','V','\0'};
	wchar_t Wavexten3[4] = {'W','a','v','\0'};
	wchar_t S48xten2[4] = {'S','4','8','\0'};
	wchar_t SLGexten[4] = {'s','l','g','\0'};
	wchar_t MP2exten[4] = {'m','p','2','\0'};
	wchar_t SLGexten2[4] = {'S','L','G','\0'};
	wchar_t SLGexten3[4] = {'S','l','g','\0'};
	int re;
	__try
	{
		extentions = wcsrchr(m_SourceFile,'.');
		extentions++;
		if (wcscmp(extentions,Wavexten)==0 || wcscmp(extentions,Wavexten2)==0 || wcscmp(extentions,Wavexten3)==0)
		{
			re = FeatureExtraction_ForWav(m_SourceFile, m_DecodedFile, parentwinhand);
		}
		else if(wcscmp(extentions,S48xten)==0 || wcscmp(extentions,S48xten2)==0 || wcscmp(extentions,SLGexten)==0 || wcscmp(extentions,SLGexten2)==0 || wcscmp(extentions,SLGexten3)==0 || wcscmp(extentions,MP2exten)==0 )
		{
		//	re = FeatureExtraction_ForS48(m_SourceFile, m_DecodedFile, parentwinhand);
			//re =FeatureExtraction_ForS48_setMute(m_SourceFile, m_DecodedFile, parentwinhand);
			//re =FeatureExtraction_ForS48_setMute_rearMute_Log(m_SourceFile, m_DecodedFile, parentwinhand);
			re = FeatureExtraction_ForS48_rearMute(m_SourceFile, m_DecodedFile, parentwinhand);
		}
		else
			re = -10;
			
		return re;
	}
	__except(1)
	{
		sprintf(Log,"%s\n","FeatureExtraction_LogFile try catch error"); 
		//	LogWrite(Log,ERRORTYPE);
	//	printf("%s",Log);
		return -9;
		
	}
}

COS_HANDLE __stdcall MergeFeExtraction_initialize(unsigned long samplerate, int channels, int chn_process)
{
	WavbufData *WavBufData = malloc(sizeof(WavbufData));
	WavBufData->samplerate = samplerate;
	WavBufData->channel = channels;
	if (channels == 1)
	{
		chn_process = 1;
	}
	if(samplerate == 48000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_48k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_48k*channels/chn_process;		
	}
	else if (samplerate == 44100)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_44k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_44k*channels/chn_process;
	}
	if(samplerate == 32000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_32k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_32k*channels/chn_process;		
	}
	else if (samplerate == 8000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_8k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_8k*channels/chn_process;
	}
	WavBufData->step =	(int)(WavBufData->BufNum * FE_DIV_IN_FRAME);	
	WavBufData->processtimes = 0;
	WavBufData->LeftSize = 0;
	WavBufData->mpa_dec_handle = MP1L2_Dec_initialize(0,0,0);
	return (COS_HANDLE)WavBufData;
}

COS_VOID __stdcall MergeFeExtraction_uninitialize(COS_HANDLE Header)
{
	WavbufData *WavBufData;
	WavBufData = (WavbufData *)Header;
	MP1L2_Dec_uninitialize(WavBufData->mpa_dec_handle);

	free(WavBufData);
}

// int __stdcall MergeFeExtraction(LPTSTR SourceFile1,LPTSTR SourceFile2, LPTSTR MergeFile)
// {
// 	COS_HANDLE handle;
// 	FILE *file1;
// 	FILE *file2;
// 	TCHAR file1fe[255];
// 	TCHAR file2fe[255];
// 
// 	wsprintf(file1fe,_T("%s%s"),SourceFile1,_T(".fe"));
// 	wsprintf(file2fe,_T("%s%s"),SourceFile2,_T(".fe"));
// 
// 	if(!SourceFile1||!SourceFile2)
// 		return -1;
// 	if ((file1 = _wfopen(SourceFile1, _T("rb"))) == NULL) 
// 	{	
// 		return -2; 
//     }
// 	if ((file2 = _wfopen(SourceFile2, _T("rb"))) == NULL)
// 	{
// 		fclose(SourceFile1);
// 		return -3; 
// 	}
// 
// 	handle = MergeFeExtraction_initialize(48000,2,2);
// 
// 	MergeFeExtraction_ForS48(handle, SourceFile1, file1fe);
// 	MergeFeExtraction_ForS48(handle, SourceFile2, file2fe);
// 	
// 	MergeFeExtraction_uninitialize(handle);
// 
// 	return 0;
// }

int __stdcall MergeFeExtraction_ForS48(COS_HANDLE handle, LPTSTR SourceFile,LPTSTR TargetFile)
{
	WavbufData* Handle;
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
	short       WavBuf[WAVSIZE_48k];
	BYTE       	MpaBuf[INSIZE_mp2];
    int sizeofshort = sizeof(short);
	int CurLeftPcmNum = 0;
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;

	int FeDataNum;
	int FeDataNum_2pow;
	int Fesize = sizeof(float);
	float* pFe;
	short* pWav;
	int FeStep;
	int FeProcessTimes = 0;
	float *MFCC, *pMFCC;
	float frameA[2*MPEG_SAMPLE_IN_FRAME_48k*11]; 

	FILE  * decodefile;
	FILE  * sourcefile;

	BYTE	*pin;
	BYTE	*pout;

	int  outsize=0;
	int  insize=0;
	long totalread=0;

    unsigned long  codech;
	MPADecodeContext *s;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;
	
	int kk = 0;

	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	if(!SourceFile||!TargetFile)
		return -1;

	if ((sourcefile = _wfopen(SourceFile, _T("rb"))) == NULL) 
	{	
		return -2; 
    }
	if ((decodefile = _wfopen(TargetFile, _T("w+b"))) == NULL)
	{
		fclose(sourcefile);
		return -3; 
	}
	
	Handle = (WavbufData*)handle;
	
	memcpy(WavBuf, Handle->LeftBuff, Handle->LeftSize * sizeofshort);   //连续文件,上一个文件尾半帧
	CurLeftPcmNum = Handle->LeftSize;
	//insize = CurLeftPcmNum/576*384;
	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile);
	fseek(sourcefile,0,SEEK_SET);

	MFCC = (float *)malloc(Fesize * FE_CEP_ORDER_ADD1 * WRITELIMIT + 1024);
	pMFCC = MFCC;

    codech = Handle->mpa_dec_handle;
    s = (MPADecodeContext*)codech;
	countFrame = 0;

    while(!feof(sourcefile))
	{
		unsigned int real_read;
		kk++;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf+CurLeftPcmNum);
        wanttoread = INSIZE_mp2 - insize;
        decodewavesize = 0;

		real_read = fread(pin+insize,sizeof(BYTE),wanttoread,sourcefile);
		if(real_read == wanttoread)
			insize = INSIZE_mp2;
		else
			insize = insize + real_read;
	
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread;
		
		while(insize>0)
		{
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&outsize);//解码
			if (curencodempasize<=0)
			{
				free(MFCC);
				fclose(sourcefile);
    			fclose(decodefile);
				printf("MP1L2_Dec_codec failed\n");
				return -4;
			}
			pin+=curencodempasize;
			insize-=curencodempasize;

			if(outsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= outsize;
			decodewavesize+=outsize;	
					
			if(insize<0) 
			{
				insize+=curencodempasize;
				break;
			}
			if(decodewavesize>=WAVSIZE_48k) 
			{			
				break;
			}	

		}
		
		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
			{
				if (s->nb_channels == 2)
				{
					Stereo2Mono(WavBuf+CurLeftPcmNum, NewSampleNum); //v1.4.3_mono
				}
				else
					chn_process = 1;
			}
			
			CurLeftPcmNum = NewSampleNum/chn_process+CurLeftPcmNum;//当前剩余未处理的采样点的个数
			CurTotalPcmNum = CurLeftPcmNum;//当前总采样点的个数
			
			
			FeDataNum =MPEG_SAMPLE_IN_FRAME_48k*s->nb_channels/chn_process;//当前处理的单元,立体声=2304
			FeDataNum_2pow = MPEG_SAMPLE_IN_FRAME_2POW_48k*s->nb_channels/chn_process;
            pFe = frameA;
			pWav = WavBuf;
			FeStep =(int)(FeDataNum * FE_DIV_IN_FRAME);//当前处理的单元的一半,立体声=2304*0.5

			while ( CurLeftPcmNum>= FeDataNum )
			{
				PCMNormalize(pFe,pWav,FeDataNum);

				mel_cepstrum_basic(pFe, FeDataNum, pMFCC, FE_CEP_ORDER, FeDataNum_2pow,s->sample_rate);	   
			
				pMFCC += FE_CEP_ORDER_ADD1;
				countFrame++;

				pWav = pWav + FeStep;//前移半帧，立体声=1152
				pFe = pFe + FeStep;
				FeProcessTimes++;
				CurLeftPcmNum -= FeStep;
			}
			//assert(CurLeftPcmNum==1152/chn_process);
			memcpy(Handle->LeftBuff, WavBuf + CurTotalPcmNum - CurLeftPcmNum, CurLeftPcmNum * sizeofshort);
			Handle->LeftSize = CurLeftPcmNum;
            memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
		}

	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

		if (countFrame >= WRITELIMIT)
		{
/********************************1000帧写一次fe**************************************/
			fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);

			countFrame = 0;
			pMFCC = MFCC;
		}
	} 
	
 	if (countFrame > 0)
 	{
 		fwrite(MFCC, Fesize, FE_CEP_ORDER_ADD1 * countFrame, decodefile);
 	}

	free(MFCC);
	fclose(sourcefile);
    fclose(decodefile);

    return 0;
}