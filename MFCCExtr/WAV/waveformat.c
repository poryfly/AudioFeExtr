/*------Handling simple (i.e. PCM format only) wav format audio file-----*/
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "waveformat.h"

void write_intel_ushort(int in, FILE *fp)
{
	unsigned char temp[SIZE_SHORT];
	temp[0] = in & 0xff;
	temp[1] = in >> 8;
	fwrite(temp, sizeof(*temp), SIZE_SHORT, fp);
}


void write_intel_ulong(unsigned long in, FILE *fp)
{
	unsigned char temp[SIZE_LONG];
	temp[0] = in & 0xff;
	in >>= 8;
	temp[1] = in & 0xff;
	in >>= 8;
	temp[2] = in & 0xff;
	in >>= 8;
	temp[3] = in & 0xff;
	fwrite(temp, sizeof(*temp), SIZE_LONG, fp);
}


unsigned short read_intel_ushort(FILE *fp)
{
	unsigned short cx;
	unsigned char temp[SIZE_SHORT];
	fread(temp, sizeof(*temp), SIZE_SHORT, fp);
	cx = temp[0] | (temp[1] * 256);
	return cx;
}


unsigned long read_intel_ulong(FILE *fp)
{
	unsigned long cx;
	unsigned char temp[SIZE_LONG];
	fread(temp, sizeof(*temp), SIZE_LONG, fp);
	cx =  (unsigned long)temp[0];
	cx |= (unsigned long)temp[1] << 8;
	cx |= (unsigned long)temp[2] << 16;
	cx |= (unsigned long)temp[3] << 24;
	return cx;
}
waveFormat readWaveHeader(FILE *fp)
{
	long nread, nskip, x_size,format;
	int except = 0;
	unsigned short channels,bytes_per_sec, block_align;
	unsigned long frequency;
	unsigned long bits_per_sample;
	unsigned long bytes_per_sample;
	unsigned long data_size;
	unsigned char temp[SIZE_ID];
	unsigned char *skip;
	waveFormat fmt;
//	unsigned long head_size=0; //jhy 08-9-21
	skip  = NULL;
	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
//	head_size+=SIZE_ID;
	if (memcmp(temp, "RIFF", (size_t)SIZE_ID)!=0)
	{
		fprintf(stderr, "File is not WAVE format!\n");
		exit(0);
		//return (waveFormat)(-1);
		//except = 1/except;
		//return NULL;
	}
	nread = fread(temp, sizeof(*temp), SIZE_LONG, fp);
//	head_size+=SIZE_ID;


	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
//	head_size+=SIZE_ID;
	if (memcmp(temp, "WAVE", (size_t)SIZE_ID)!=0)
	{
		fprintf(stderr, "File is not WAVE format!\n");
		exit(0);
		//except = 1/except;
	}

	while(1)
	{
		nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
	//	head_size+=SIZE_ID;

		/*-- skip chunks except for "fmt " or "data" -----*/
	
		if(memcmp(temp, "LIST", (size_t)SIZE_ID)==0)
			//	while((memcmp(temp, "LIST ", (size_t)SIZE_ID)!=0))
		{
			nskip = read_intel_ulong(fp);
		//	head_size+=nskip;
			if (nskip!=0)			
				fseek(fp, nskip, SEEK_CUR);
		
			
		continue;
		}
	//	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
		/*-- skip chunks except for "fmt " or "data" -----*/
	
		if(memcmp(temp, "fmt ", (size_t)SIZE_ID)==0)
			//	while((memcmp(temp, "LIST ", (size_t)SIZE_ID)!=0))
		{  /*{
			nskip = read_intel_ulong(fp);
			if (nskip!=0)
			{
				fseek(fp, nskip, SEEK_CUR);
			}
		}*/
		x_size = read_intel_ulong(fp);
	//	head_size+=x_size;
		format = read_intel_ushort(fp);
	//	head_size+=format;
		x_size -= SIZE_SHORT;
		if (WAVE_FORMAT_PCM2 != format)
		{
			fprintf(stderr, "Error! Unsupported WAVE File format.\n");
			exit(0);
		}
		channels = read_intel_ushort(fp);
	//	head_size+=channels;
		x_size -= SIZE_SHORT;
		frequency = read_intel_ulong(fp);

		x_size -= SIZE_LONG;
		bytes_per_sec   = read_intel_ulong(fp);  /* skip bytes/s */
		block_align     = read_intel_ushort(fp); /* skip block align */
		bits_per_sample = read_intel_ushort(fp); /* bits/sample */
		bytes_per_sample= (bits_per_sample + BITS_PER_BYTE - 1)/BITS_PER_BYTE;
		block_align     = bytes_per_sample * channels;
		x_size -= SIZE_LONG + SIZE_SHORT + SIZE_SHORT;  /*-- skip additional part of "fmt " header -----*/
		if (x_size!=0)		
			fseek(fp, x_size, SEEK_CUR);

		continue;
		
		}
	//	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
		/*-- skip chunks except for "fmt " or "data" -----*/
		if(memcmp(temp, "fact", (size_t)SIZE_ID)==0)
		{
			nskip = read_intel_ulong(fp);
			if (nskip!=0)		
				fseek(fp, nskip, SEEK_CUR);

			continue;
			
		}
    
	if(memcmp(temp, "data", SIZE_ID)==0)
// 		{
// 			nskip = read_intel_ulong(fp);  /* read chunk size */
// 			fseek(fp, nskip, SEEK_CUR);
// 			nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
// 		}
	{
		data_size = read_intel_ulong(fp);
    	break;
	}
    
	nskip = read_intel_ulong(fp);  //如果不是所有的情况，skip
	if (nskip!=0)		
	   fseek(fp, nskip, SEEK_CUR);

	}
// 	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
// 					/*-- skip chunks except for "fmt " or "data" -----*/
// 	while(memcmp(temp, "fmt ", (size_t)SIZE_ID)!=0)
// 	{
// 		nskip = read_intel_ulong(fp);
// 		if (nskip!=0)
// 		{
// 			fseek(fp, nskip, SEEK_CUR);
// 		}
// 	}
// 	x_size = read_intel_ulong(fp);
// 	format = read_intel_ushort(fp);
// 	x_size -= SIZE_SHORT;
// 	if (WAVE_FORMAT_PCM != format)
// 	{
// 		fprintf(stderr, "Error! Unsupported WAVE File format.\n");
// 		exit(0);
// 	}
// 	channels = read_intel_ushort(fp);
// 	x_size -= SIZE_SHORT;
// 	frequency = read_intel_ulong(fp);
// 	x_size -= SIZE_LONG;
// 	bytes_per_sec   = read_intel_ulong(fp);  /* skip bytes/s */
// 	block_align     = read_intel_ushort(fp); /* skip block align */
// 	bits_per_sample = read_intel_ushort(fp); /* bits/sample */
// 	bytes_per_sample= (bits_per_sample + BITS_PER_BYTE - 1)/BITS_PER_BYTE;
// 	block_align     = bytes_per_sample * channels;
// 	x_size -= SIZE_LONG + SIZE_SHORT + SIZE_SHORT;  /*-- skip additional part of "fmt " header -----*/
// 	if (x_size!=0)
// 	{
// 		fseek(fp, x_size, SEEK_CUR);
// 	}

	/*----- skip chunks except for "data" -----*/
// 	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
// 	while(memcmp(temp, "data", SIZE_ID)!=0)
// 	{
// 		nskip = read_intel_ulong(fp);  /* read chunk size */
// 		fseek(fp, nskip, SEEK_CUR);
// 		nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
// 	}
// 	data_size = read_intel_ulong(fp);

	fmt.format           = format;
	fmt.channels         = channels;
	fmt.frequency        = frequency;
	fmt.bytes_per_sample = bytes_per_sample;
	fmt.block_align      = block_align;
	fmt.data_size        = data_size / bytes_per_sample; /* byte to short */
	return fmt;
}
void FileReadWaveHeader(char *file,int* bps,int* chs,int *freq)////2004-11-04-v1.1
{
	long nread, nskip, x_size,format;
	int except = 0;
	unsigned short channels,bytes_per_sec, block_align;
	unsigned long frequency;
	unsigned long bits_per_sample;
	unsigned long bytes_per_sample;
	unsigned long data_size;
	unsigned char temp[SIZE_ID];
	unsigned char *skip;
	waveFormat fmt;	
	FILE* fp;

	skip  = NULL;

   if((fp = fopen(file,"rb"))==NULL)
   {
	   //MessageBox(0,"sourcefile file can not open","ff",MB_OK);
	   return NULL;
   }

	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
	if (memcmp(temp, "RIFF", (size_t)SIZE_ID)!=0)
	{
		fprintf(stderr, "File is not WAVE format!\n");
		exit(0);
		//except = 1/except;
	}
	nread = fread(temp, sizeof(*temp), SIZE_LONG, fp);

	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
	if (memcmp(temp, "WAVE", (size_t)SIZE_ID)!=0)
	{
		fprintf(stderr, "File is not WAVE format!\n");
		exit(0);
		//except = 1/except;
	}
	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
					/*-- skip chunks except for "fmt " or "data" -----*/
	while(memcmp(temp, "fmt ", (size_t)SIZE_ID)!=0)
	{
		nskip = read_intel_ulong(fp);
		if (nskip!=0)
		{
			fseek(fp, nskip, SEEK_CUR);
		}
	}
	x_size = read_intel_ulong(fp);
	format = read_intel_ushort(fp);
	x_size -= SIZE_SHORT;
	if (WAVE_FORMAT_PCM2 != format)
	{
		fprintf(stderr, "Error! Unsupported WAVE File format.\n");
		exit(0);
		//except = 1/except;
	}
	channels = read_intel_ushort(fp);
	x_size -= SIZE_SHORT;
	frequency = read_intel_ulong(fp);
	x_size -= SIZE_LONG;
	bytes_per_sec   = read_intel_ulong(fp);  /* skip bytes/s */
	block_align     = read_intel_ushort(fp); /* skip block align */
	bits_per_sample = read_intel_ushort(fp); /* bits/sample */
	bytes_per_sample= (bits_per_sample + BITS_PER_BYTE - 1)/BITS_PER_BYTE;
	block_align     = bytes_per_sample * channels;
	x_size -= SIZE_LONG + SIZE_SHORT + SIZE_SHORT;  /*-- skip additional part of "fmt " header -----*/
	if (x_size!=0)
	{
		fseek(fp, x_size, SEEK_CUR);
	}

	/*----- skip chunks except for "data" -----*/
	nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
	while(memcmp(temp, "data", SIZE_ID)!=0)
	{
		nskip = read_intel_ulong(fp);  /* read chunk size */
		fseek(fp, nskip, SEEK_CUR);
		nread = fread(temp, sizeof(*temp), SIZE_ID, fp);
	}
	data_size = read_intel_ulong(fp);

	fmt.format           = format;
	fmt.channels         = channels;
	fmt.frequency        = frequency;
	fmt.bytes_per_sample = bytes_per_sample;
	fmt.block_align      = block_align;
	fmt.data_size        = data_size; /* byte to short */

	*bps=bytes_per_sample;
	*chs=channels;
	*freq=frequency;
}

/*------------------------------------------------------------------------*/
/*--------sequential block read of wavfile--------------------------------*/
/*--------This routine can handle only "PCM" Format-----------------------*/
/*--------calling sequence is similar to that of "fread"------------------*/
/*------------------------------------------------------------------------*/
/* int readwav( long int   *is    : read samples                          */
/*         	int  length       : number of samples to be read          */
/*		waveFormat fmt    : wave format (have read from the FILE) */
/*         	FILE   *fp        : the FILE Pointer                      */
/*            )                                                           */


/*------------------------------------------------------------------------------------*/

int readwav(long int *is, int length, waveFormat fmt, FILE *fp)
{
	unsigned char *temp;
	long int i, j, k, n_sample, ix, nread, lsize, in;
	double mult, x;
	lsize = length * fmt.bytes_per_sample * fmt.channels;
	temp = (unsigned char *)malloc(sizeof(*temp) * lsize);
	if (NULL == temp)
	{
		fprintf(stderr, "Unrecoverble error ! read malloc failed\n");
		exit(1);
	}
	nread = fread(temp, sizeof(unsigned char), lsize, fp);
	n_sample = 0;
	if (1 != fmt.bytes_per_sample)
	{
		for (i = 0; i < nread; i += fmt.block_align)
		{      /**** 1 bytes per sample is the exceptional case ****/
			for (j = 0; j < fmt.channels; ++j)
			{
				in = i + j * fmt.bytes_per_sample;
				mult = 1.0; 
				x = 0.0;
				for (k = 0; k < fmt.bytes_per_sample - 1; ++k)
				{
					x += (double)temp[in + k] * mult;
					mult *= 256.0;
				}
				if (128 < temp[in + k])
				{ /* the last byte of the sample */
					x += (double)(temp[in + k] - 256) * mult;
				}
				else
				{
					x += (double)temp[in+k] * mult;
				}
				ix = (long)x;
				is[n_sample++] = ix;
			}
		}
	}
	else
	{
		for (i = 0; i < nread; i += fmt.block_align)
		{
			for (j = 0; j < fmt.channels; ++j)
			{
				ix = (temp[i+j] - 128) * 256;   /* convert into long */
				is[n_sample++] = ix;
			}
		}
	}
	free(temp);
	return n_sample;
}


/*-----------------------------------------------------------------------------------------------*/
/* sequential block write of wavfile                                  		*/
/* This routine can handle only "PCM" Format                            	*/
/* calling sequence is similar to that of "fwrite"                       	*/
/* int writewav(long int   *is    : write samples                         	*/
/*             	int length        : number of samples to be read          	*/
/*         	waveFormat fmt    : wave format (have read from the FILE) 	*/
/*         	FILE *fp          : the FILE Pointer                       	*/
/*             )                                                      		*/  	
/*----------------------------------------------------------------------------------------------*/


int writewav(long *is, int length, waveFormat fmt, FILE *fp)
{
	signed char *temp;
	int i, j, k, in, lsize, wsize;
	long ix, itmp;
	lsize = length * fmt.bytes_per_sample * fmt.channels;
	temp = (signed char *)malloc(sizeof(*temp) * lsize);
	if (NULL == temp)
	{
		fprintf(stderr, "Unrecoverble error ! write malloc failed\n");
		exit(1);
	}
	if (1 != fmt.bytes_per_sample)
	{
		for (i = 0; i < length; ++i)
		{
			for (j = 0; j < fmt.channels; ++j)
			{
				in = i * fmt.channels + j;
				ix = is[in];
				for (k = 0; k < fmt.bytes_per_sample; ++k)
				{
					itmp = ix & 255;
					temp[in*fmt.bytes_per_sample+k] =(char)itmp;
					ix >>= 8;
				}
			}
		}
	}
	else
	{
		for (i = 0; i < length; ++i)
		{
			in = i*fmt.channels;
			for (j = 0; j < fmt.channels; ++j)
			{
				temp[in+j] = is[in+j]/256+128;  
			}
		}
	}
	wsize = fwrite(temp, sizeof(*temp), lsize, fp);
	free(temp);
	return wsize/fmt.block_align;
}

/*--------------------------------------------------------------*/
/*---------------- Write minimal Wave Header--------------------*/
/* RIFF                                       4 */
/* chunk size                                 4 */
/* 16+12+8 = 36                                 */
/* WAVE                                       4 */
/* fmt                                        4 */
/* chunk size (fmt chunk size is ever 16)     4 */
/*                                              */
/* 16                                           */
/* format type                                2 */
/* number of channels                         2 */
/* sampleing frequency                        4 */
/* bytes per second                           4 */
/* block align per                            2 */
/* bits per sample                            2 */
/*                                              */
/* 8                                            */
/* data                                       4 */
/* data chunk size in bytes                   4 */
/*-----------------------------------------------------------------------*/

void writeWaveHeader(waveFormat fmt, FILE *fp)
{
	unsigned short block_align;
	unsigned short bits_per_sample;
	unsigned long samples_per_sec;
	unsigned long bytes_per_sec;
	unsigned long bytes_total_data;
	unsigned long total_header_size; /* except RIFF chunk */
	unsigned long fmt_header_size;

	samples_per_sec  = fmt.channels         * fmt.frequency;
	bytes_per_sec    = samples_per_sec      * fmt.bytes_per_sample;
	block_align      = fmt.channels         * fmt.bytes_per_sample;
	bits_per_sample  = fmt.bytes_per_sample * BITS_PER_BYTE;
	bytes_total_data = fmt.data_size        * block_align;
	fmt_header_size  = SIZE_SHORT * 4  + SIZE_LONG *2;
	total_header_size=fmt_header_size+SIZE_ID*4+SIZE_LONG+bytes_total_data; /*  16+16+data size+rest */

	fseek(fp, 0, SEEK_SET);
	fwrite("RIFF", sizeof(char), SIZE_ID, fp);
	write_intel_ulong(fmt.data_size*fmt.block_align+36, fp);
	fwrite("WAVE", sizeof(char), SIZE_ID, fp);
	fwrite("fmt ", sizeof(char), SIZE_ID, fp);
	write_intel_ulong(fmt_header_size,    fp);
	write_intel_ushort(fmt.format,        fp);
	write_intel_ushort(fmt.channels,      fp);
	write_intel_ulong(fmt.frequency,      fp);
	write_intel_ulong(bytes_per_sec,      fp);
	write_intel_ushort(block_align,       fp);
	write_intel_ushort(bits_per_sample,   fp);
	fwrite("data", sizeof(char), SIZE_ID, fp);
	write_intel_ulong(fmt.data_size * fmt.block_align, fp);
}


