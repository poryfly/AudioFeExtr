#include "feExtr.h"

void PCMNormalize(double* frameA, const short* wavpcm,unsigned long len)
{
	unsigned long i;
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
        frameA[i] =((double)wavpcm[i]*cep_max);
	}
	}
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

int * GetfreqBandToBin(int bandum, int startf, int endf,int framelen,int fs)
{
	double log_startf;
	double log_endf;
	double log_dist;
	double temp;
	int i;
	int *f = (int*)malloc((bandum+1)*sizeof(int));
	log_startf = log10(startf);
	log_endf = log10(endf);
	log_dist = log_endf - log_startf;
	f[0] = floor(framelen*startf/fs);
	for (i=1; i<=bandum; i++)
	{
		temp = pow(10,(i*log_dist/bandum + log_startf));
		f[i] = floor(temp*framelen/fs);
	}
	return f;
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
	else if(samplerate == 4000)
	{
		WavBufData->fft_size = MPEG_SAMPLE_IN_FRAME_2POW_4k*channels/chn_process;
		WavBufData->BufNum = MPEG_SAMPLE_IN_FRAME_4k*channels/chn_process;
	}
	
 	WavBufData->processtimes = 0;
	WavBufData->LeftSize = 0;
	WavBufData->forsearch = 0;
	WavBufData->FrameLen = 256;
	WavBufData->targf = 4000;
	WavBufData->distunit = 0.151496;
	WavBufData->step =	(int)(WavBufData->FrameLen * FE_DIV_IN_FRAME);
	WavBufData->frequences = GetfreqBandToBin(BANDUM,STARTF,ENDF,WavBufData->FrameLen,WavBufData->targf);
 	return (COS_HANDLE)WavBufData;
}
COS_VOID __stdcall FeatureExtraction_uninitialize(COS_HANDLE Header)
{
	WavbufData *WavBufData;
	WavBufData = (WavbufData *)Header;
	free(WavBufData->frequences);
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


char *resample(char *inBuf, int targf, int samplerate, int insize,int *outsize)
{
	uintptr_t h_resflt;
	int is_last = 0;
	int in_len_bytes     = 0;
    int out_len_bytes    = 0;
    int read_len         = 0;
	char * inBufTemp = NULL;
	char * outBuf = (char *)malloc(insize+100);
	char * outBufTemp = NULL;
	short wavsamples_in[FA_FRAMELEN_MAX];
	short wavsamples_out[FA_FRAMELEN_MAX];
    unsigned char * p_wavin  = (unsigned char *)wavsamples_in;
    unsigned char * p_wavout = (unsigned char *)wavsamples_out;
	inBufTemp = inBuf;
	outBufTemp = outBuf;
	read_len = insize;
	opt_downfactor = samplerate /targf;
	h_resflt = fa_decimate_init(opt_downfactor, opt_gain, BLACKMAN);
	in_len_bytes = fa_get_resample_framelen_bytes(h_resflt);
	while(1)
    {
        if(is_last)
            break;
		
        memset(p_wavin, 0, in_len_bytes);
		memcpy(p_wavin,inBufTemp,in_len_bytes);
        read_len = read_len-in_len_bytes;
        if(read_len < in_len_bytes)
            is_last = 1;
		
      	fa_decimate(h_resflt, p_wavin, in_len_bytes, p_wavout, &out_len_bytes);   
		memcpy(outBufTemp,p_wavout,out_len_bytes);
		outBufTemp = outBufTemp + out_len_bytes;
		inBufTemp = inBufTemp+in_len_bytes;
		(*outsize) += out_len_bytes;
	}
	
	return outBuf;
}



COS_DWORD __stdcall FeatureExtraction_Buf_Discard(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize)  //残余丢弃
{
	int FrameCount;
	int new_insize;
	char* inBuf_tmp = NULL;
	short* Wavbuf;
	int Fesize = sizeof(float);
	double *filtMgt = NULL;
	uintptr_t ffthandle;
	//float *pFe = malloc(insize * Fesize + 100); 
	double *pFe = malloc(insize * Fesize + 100); 
	float *tmpFe=pFe;
	WavbufData *WavBufData;
	int curleftsize;
	*outsize = 0;
	new_insize = 0;
	WavBufData = (WavbufData *)Handle;
	Wavbuf = (short*)inBuf;
	if (WavBufData->channel == 2)
	{
		Stereo2Mono(Wavbuf, insize/2); 
		insize /= 2;
	}
	if (WavBufData->samplerate != WavBufData->targf)
	{
		inBuf_tmp = resample(inBuf,WavBufData->targf,WavBufData->samplerate,insize,&new_insize);
		WavBufData->samplerate = WavBufData->targf;
	}
	else
	{
		inBuf_tmp = malloc(insize + 100);
		memcpy(inBuf_tmp, inBuf, insize);
		new_insize = insize;
	}


	Wavbuf = (short*)inBuf_tmp;

	//PCMNormalize(pFe,Wavbuf,insize/2);
	
	//filtMgt = WavBufData->filtMg;
// 	if (WavBufData->channel == 2)
// 	{
// 		Stereo2Mono(Wavbuf, insize/2); 
// 		insize /= 2;
// 	}
	curleftsize = new_insize/2;   //curleftsize 采样点数
	FrameCount = 0;
	//ffthandle = fa_fft_init(WavBufData->fft_size);
	while(curleftsize >= WavBufData->FrameLen)
	{
		PCMNormalize(pFe,Wavbuf,WavBufData->FrameLen);
		Wavbuf += WavBufData->step;
		//mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);
		hffcoef(pFe, WavBufData->FrameLen, outBuf, FE_CEP_ORDER,WavBufData->frequences);
		pFe = pFe + WavBufData->step;
		curleftsize -= WavBufData->step;
		*outsize = *outsize + FE_CEP_ORDER * Fesize;
		outBuf += FE_CEP_ORDER;
		FrameCount++;
	}
	//fa_fft_uninit(ffthandle);
	free(tmpFe);
	free(inBuf_tmp);
	return (insize - curleftsize * 2);
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
	short       WavBuf[WAVSIZE_48k];
	int sizeofshort = sizeof(short);
	int CurTotalPcmNum = 0;
	int NewSampleNum = 0;
	int Fesize = sizeof(float);
	int FeProcessTimes = 0;
    float Hf_cep[FE_CEP_ORDER*20];
	int  HfccSize;
	int  tmp;

	FILE  * decodefile;
	FILE  * sourcefile;
	
	BOOL BProcess;
	BYTE	*pout;	
	int  outsize=0;

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
	fmt = readWaveHeader(sourcefile);
	if (fmt.frequency == 48000)
	{
		wanttoread =WAVSIZE_48k/2; 
	}
	else if (fmt.frequency == 44100)
	{
		wanttoread = WAVSIZE_44k/2;
	}
	else if (fmt.frequency == 32000)
	{
		wanttoread = WAVSIZE_32k/2;
	}
	else if (fmt.frequency == 8000)
	{
		wanttoread = WAVSIZE_8k/2;
	}
	else if (fmt.frequency == 4000)
	{
		wanttoread = WAVSIZE_4K/2;
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
		

	WavbufHeader = FeatureExtraction_initialize(fmt.frequency,fmt.channels, chn_process);
	((WavbufData*)WavbufHeader)->LeftSize = 0;
	while(!feof(sourcefile))
	{
		unsigned int real_read;
		pout =(unsigned char *)(WavBuf+((WavbufData*)WavbufHeader)->LeftSize);
		BProcess=CheckMessageQueue();
		//real_read = fread(pout,2,wanttoread,sourcefile);
		real_read = fread(pout,2,wanttoread,sourcefile);
		totalread+=real_read; //jhy 08-4-10
		m_lcurfilelen=totalread * sizeofshort;		
		m_process=(unsigned int)(((double)(m_lcurfilelen)/(m_ltotalfilelen))*100);  //		
		PostMessage(m_parentwinhand,WM_PROCESS_MESSAGE,0,m_process);  //jhy 08-4-10
		if(real_read > wanttoread/10)
		{
			NewSampleNum = real_read;//新产生的采样点个数
			CurTotalPcmNum = NewSampleNum + ((WavbufData*)WavbufHeader)->LeftSize;
					//mlgb = CurTotalPcmNum;
			tmp = NewSampleNum / ((WavbufData*)WavbufHeader)->channel + ((WavbufData*)WavbufHeader)->LeftSize;
			//FeatureExtraction_Buf_Continuous(WavbufHeader,(char*)WavBuf, CurTotalPcmNum,T_mel_cep,&MfccSize);
			FeatureExtraction_Buf_Discard(WavbufHeader,(char *)WavBuf, CurTotalPcmNum*2,Hf_cep,&HfccSize);
			memcpy(WavBuf,WavBuf + tmp - ((WavbufData*)WavbufHeader)->LeftSize, ((WavbufData*)WavbufHeader)->LeftSize*sizeof(short));
			fwrite(Hf_cep,Fesize,HfccSize/Fesize,decodefile);
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




int __stdcall FeatureExtraction_ForS48(LPTSTR m_SourceFile,LPTSTR m_DecodedFile,long *parentwinhand)
{
	long m_lcurfilelen=0;
    long m_ltotalfilelen=0;
    BOOL m_bjobdone=FALSE;
    HWND m_parentwinhand;
	COS_HANDLE WavbufHeaderHandle;
	WavbufData *WavBufData;
	unsigned int m_process=0;  //jhy 08-4-10
	short       WavBuf[WAVSIZE_48k];
	BYTE       	MpaBuf[INSIZE_mp2];
    int sizeofshort = sizeof(short);
	int CurTotalPcmNum;
	int NewSampleNum;
	int chn_process = 2;
	int countFrame;

	int Fesize = sizeof(float);
	short* pWav;
	int FeStep;
	int FeProcessTimes = 0;
    float T_mel_cep[FE_CEP_ORDER];
	float *HFFCC, *pHFFCC;
	//float frameA[2*MPEG_SAMPLE_IN_FRAME_48k*11]; 
	//int frame_size=960;
    wchar_t m_Data[512];
	FILE  * decodefile = NULL;
	FILE  * sourcefile = NULL;
	BOOL BProcess;
	BYTE	*pin;
	BYTE	*pout;

	int  DecOutsize=0;
	int FeOutsize=0;
	int HfccFesize;
	int  insize=0;
	long totalread=0;
    unsigned long  codech;
	MPADecodeContext *s = NULL;

	DWORD count = 0,inc=0,slice=0,step=0;
	DWORD decodewavesize=0,totalwavesize=0;
	DWORD curencodempasize=0;
	DWORD wanttoread=INSIZE_mp2;


	//初始化
	m_bjobdone=FALSE;
	m_lcurfilelen=0;
	m_ltotalfilelen=0;

	wsprintf(m_Data,_T("%s%s"),m_SourceFile,_T("_m.pcm"));
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


	fseek(sourcefile,0,SEEK_END);
	m_ltotalfilelen=ftell(sourcefile);
	fseek(sourcefile,0,SEEK_SET);

	HFFCC = (float *)malloc(Fesize * FE_CEP_ORDER * WRITELIMIT + 1024);

	pHFFCC = HFFCC;
	m_parentwinhand=(HWND)parentwinhand;   //.jhy 08-4-10

    codech = MP1L2_Dec_initialize(0,0,0);
    s = (MPADecodeContext*)codech;
	countFrame = 0;

    while(!feof(sourcefile))
	{
		unsigned int real_read;

		pin = MpaBuf;

        pout =(unsigned char *)(WavBuf);
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
			curencodempasize=MP1L2_Dec_codec(codech,pin,insize,pout,&DecOutsize);//解码
			pin+=curencodempasize;
			insize-=curencodempasize;

			

			
			if(DecOutsize<=0) 
			{ 
				break;/////maybe there insize bytes in buffer don't be decoded
			}	

			pout+= DecOutsize;
			decodewavesize+=DecOutsize;	
					
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
		
		WavbufHeaderHandle = FeatureExtraction_initialize(s->sample_rate,s->nb_channels, chn_process);
		WavBufData = (WavbufData *)WavbufHeaderHandle;

		if(decodewavesize>0)
		{
            NewSampleNum = decodewavesize/sizeofshort;//新产生的采样点个数
			
// 			if (chn_process == 2)//是否选立体声转单声道处理，要处理时=2
// 			{
// 				if (WavBufData->channel == 2)
// 				{
// 					Stereo2Mono(WavBuf, NewSampleNum); //v1.4.3_mono
// 					WavBufData->channel = 1;
// 				}
// 				else
// 					chn_process = 1;
// 			}
			
//			CurTotalPcmNum = NewSampleNum/chn_process;//当前总采样点的个数
			CurTotalPcmNum = NewSampleNum;
			pWav = WavBuf;
			FeatureExtraction_Buf_Discard(WavbufHeaderHandle,(char *)pWav, CurTotalPcmNum*2,pHFFCC,&FeOutsize);
			countFrame += FeOutsize/(Fesize*FE_CEP_ORDER);
			//assert(CurLeftPcmNum==1152/chn_process);
            //memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
		}

	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

		if (countFrame >= WRITELIMIT)
		{
/********************************1000帧写一次fe**************************************/
			fwrite(HFFCC, Fesize, FE_CEP_ORDER * countFrame, decodefile);

			countFrame = 0;
			pHFFCC = HFFCC;
		}

		FeatureExtraction_uninitialize(WavbufHeaderHandle);
	} 
	
	if (countFrame > 0)
	{
		fwrite(HFFCC, Fesize, FE_CEP_ORDER * countFrame, decodefile);
	}

	
	free(HFFCC);
	fclose(sourcefile);
    fclose(decodefile);
	MP1L2_Dec_uninitialize(codech);

	m_bjobdone=TRUE;

	//jhy 08-4-10
	PostMessage(m_parentwinhand,WM_PROCESS_FINISH,0,100);

    return 0;
}








int __stdcall FeatureExtraction_File(LPTSTR m_SourceFile,LPTSTR m_DecodedFile, long *parentwinhand)
{
	wchar_t* extentions;
	wchar_t Wavexten[4] = {'w','a','v','\0'};
	wchar_t S48xten[4] = {'s','4','8','\0'};
	wchar_t Wavexten2[4] = {'W','A','V','\0'};
	wchar_t Wavexten3[4] = {'W','a','v','\0'};
	wchar_t S48xten2[4] = {'S','4','8','\0'};
	int re;
	extentions = wcsrchr(m_SourceFile,'.');
	extentions++;
	if (wcscmp(extentions,Wavexten)==0 || wcscmp(extentions,Wavexten2)==0 || wcscmp(extentions,Wavexten3)==0)
	{
		re = FeatureExtraction_ForWav(m_SourceFile, m_DecodedFile, parentwinhand);
	}
	else if(wcscmp(extentions,S48xten)==0 || wcscmp(extentions,S48xten2)==0)
	{
		re = FeatureExtraction_ForS48(m_SourceFile, m_DecodedFile, parentwinhand);
	}
	else
		re = -10;
	
	return re;
}
