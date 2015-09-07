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


double *filterDesign(int fs, int FrameLen, int minfrq,int maxfrq,int fftsize )
{
	int PreFilterNum;
	double PreFilterSpace;
	double a,b,c;
	double fl[100];
	double fh[100];
	double fc[100];
	double jfreq;
	double fd[FE_CEP_ORDER];
	int i,j;
	double *filtMgt = (double*)malloc(FE_CEP_ORDER*(fftsize/2+100)*sizeof(double));
	double Erb_A,Erb_B,Erb_C,Erb_Bend,Erb_Cend;
	PreFilterNum = 8;
	PreFilterSpace = 1000.0/PreFilterNum;
	fl[0] = minfrq;
	a = 6.23*pow(10,(-6));
	b = 93.39*pow(10,(-3));
	c = 28.52;
	i = 0;
	while(fl[i] < maxfrq)
	{
		if (fl[i] < 1000)
		{
			fc[i] = fl[i] + PreFilterSpace;
			fh[i] = fl[i] + 2*PreFilterSpace;
		}
		else
		{
			Erb_A = 0.5*(1/(700+fl[i]));
			Erb_B = 700/(700+fl[i]);
			Erb_C = -0.5*fl[i]*(1+700/(700+fl[i]));
			Erb_Bend = (b - Erb_B)/(a - Erb_A);
			Erb_Cend = (c - Erb_C)/(a - Erb_A);
			fc[i] = 0.5*(sqrt(Erb_Bend*Erb_Bend - 4*Erb_Cend) - Erb_Bend);
			fh[i] = fl[i] + 2*a*fc[i]*fc[i] + b*fc[i] + c;
		}
		for (j = 0; j< 0.5*fftsize; j++)
		{
			jfreq = fs*(j+1)*1.0/FrameLen;
			if (jfreq >= fl[i] && jfreq < fc[i])
			{
				*(filtMgt + (int)(i*0.5*fftsize +j)) = (jfreq - fl[i])/(fc[i] - fl[i]);
			}
			else if (jfreq >= fc[i] && jfreq<fh[i])
			{
				*(filtMgt + (int)(i*0.5*fftsize +j)) = (1 - (jfreq-fc[i])/(fh[i]-fc[i]));
			}
			else
				*(filtMgt + (int)(i*0.5*fftsize +j)) = 0;
			
		}
		fl[i+1] = fc[i];
		i++;
	}
	for (j=0; j<=PreFilterNum; j++)
	{
		fd[j] = 0.5;
	}
	j++;
	while(fd[j] <= 1)
	{
		fd[j] = 0.5+(1-0.5)/(FE_CEP_ORDER-1)*j;
		j++;
	}
	for (j=0; j<0.5*FrameLen; j++)
	{
		for(i=0; i<FE_CEP_ORDER; i++)
		{
			*(filtMgt + (int)(i*0.5*FrameLen +j)) = (*(filtMgt + (int)(i*0.5*FrameLen +j))) * fd[i];
		}
	}
	return filtMgt;
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

	WavBufData->filtMg = filterDesign(samplerate, WavBufData->BufNum, MINFRQ, MAXFRQ, WavBufData->fft_size);
	
 	return (COS_HANDLE)WavBufData;
}
COS_VOID __stdcall FeatureExtraction_uninitialize(COS_HANDLE Header)
{
	WavbufData *WavBufData;
	WavBufData = (WavbufData *)Header;
	free(WavBufData->filtMg);
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



COS_DWORD __stdcall FeatureExtraction_Buf_Discard(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize)  //残余丢弃
{
	int FrameCount;
	char* inBuf_tmp = malloc(insize + 100);
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
	
	WavBufData = (WavbufData *)Handle;
	memcpy(inBuf_tmp, inBuf, insize);

	Wavbuf = (short*)inBuf_tmp;

	//PCMNormalize(pFe,Wavbuf,insize/2);
	
	filtMgt = WavBufData->filtMg;
	if (WavBufData->channel == 2)
	{
		Stereo2Mono(Wavbuf, insize/2); 
		insize /= 2;
	}
	curleftsize = insize/2;   //curleftsize 采样点数
	FrameCount = 0;
	ffthandle = fa_fft_init(WavBufData->fft_size);
	while(curleftsize >= WavBufData->BufNum)
	{
		PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);
		Wavbuf += WavBufData->step;
		//mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);
		hffcoef(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate,filtMgt,ffthandle);
		pFe = pFe + WavBufData->step;
		curleftsize -= WavBufData->step;
		*outsize = *outsize + FE_CEP_ORDER * Fesize;
		outBuf += FE_CEP_ORDER;
		FrameCount++;
	}
	fa_fft_uninit(ffthandle);
	free(tmpFe);
	free(inBuf_tmp);
	return (insize - curleftsize * 2);
}




/*COS_DWORD __stdcall FeatureExtraction_Buf_Continuous(COS_HANDLE Handle, char *inBuf, int insize, float *outBuf, COS_DWORD *outsize)
{
	short* Wavbuf = (short*)inBuf;
	int Fesize = sizeof(float);
	int Pcmsize = sizeof(short);
	float *pFe =malloc(Fesize * insize + 100); 
	float *tmpFe=pFe;
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
		PCMNormalize(pFe,Wavbuf,WavBufData->BufNum);
		Wavbuf = Wavbuf + WavBufData->step;
		mel_cepstrum_basic(pFe, WavBufData->BufNum, outBuf, FE_CEP_ORDER, WavBufData->fft_size,WavBufData->samplerate);
		pFe = pFe + WavBufData->step;
		curleftsize -= WavBufData->step;
		*outsize = *outsize + FE_CEP_ORDER_ADD1;
		outBuf = outBuf + FE_CEP_ORDER_ADD1;
		WavBufData->processtimes++;
	}
	memcpy(WavBufData->LeftBuff,Wavbuf,curleftsize * Pcmsize);
	WavBufData->LeftSize = curleftsize;
	free(tmpFe);
	//	free(inBuf_tmp);
	return 0;
}*/





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
			fwrite(HFFCC, Fesize, FeOutsize/(Fesize), decodefile);
			pHFFCC = HFFCC;
			//assert(CurLeftPcmNum==1152/chn_process);
            //memcpy(WavBuf,WavBuf+CurTotalPcmNum-CurLeftPcmNum,CurLeftPcmNum*sizeofshort);//剩余半帧移到buf头，作为下一次数据
		}

	    if(insize>0)  //previous left  
		{
			memcpy(MpaBuf,pin,insize);
		}

// 		if (countFrame >= WRITELIMIT)
// 		{
// /********************************1000帧写一次fe**************************************/
// 			fwrite(HFFCC, Fesize, FE_CEP_ORDER * countFrame, decodefile);
// 
// 			countFrame = 0;
// 			pHFFCC = HFFCC;
// 		}

		FeatureExtraction_uninitialize(WavbufHeaderHandle);
	} 
	
// 	if (countFrame > 0)
// 	{
// 		fwrite(HFFCC, Fesize, FE_CEP_ORDER * countFrame, decodefile);
// 	}

	
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
