/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙智 (Chengdu, China)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


  filename: fa_wavfmt.c 
  version : v1.0.0
  time    : 2012/07/08 18:33 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  comment : only support pcm data now

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fa_wavfmt.h"

#define WAVE_FORMAT_UNKNOWN             (0x0000)
#define WAVE_FORMAT_PCM                 (0x0001)
#define WAVE_FORMAT_ADPCM               (0x0002)
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)
#define WAVE_FORMAT_OKI_ADPCM           (0x0010)
#define WAVE_FORMAT_IMA_ADPCM           (0x0011)
#define WAVE_FORMAT_DIGISTD             (0x0015)
#define WAVE_FORMAT_DIGIFIX             (0x0016)
#define IBM_FORMAT_MULAW                (0x0101)
#define IBM_FORMAT_ALAW                 (0x0102)
#define IBM_FORMAT_ADPCM                (0x0103)

#define SIZE_ID                         4
#define SIZE_LONG                       4
#define SIZE_SHORT                      2
#define BITS_PER_BYTE                   8


static void fa_write_u16(int in, FILE *fp)
{
	unsigned char temp[SIZE_SHORT];

	temp[0] = in & 0xff;
	temp[1] = in >> 8;
	fwrite(temp, sizeof(*temp), SIZE_SHORT, fp);
}

static void fa_write_u32(unsigned long in, FILE *fp)
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

static unsigned short fa_read_u16(FILE *fp)
{
	unsigned short cx;
	unsigned char  temp[SIZE_SHORT];

	fread(temp, sizeof(*temp), SIZE_SHORT, fp);
	cx = temp[0] | (temp[1] * 256);
	return cx;
}

static unsigned long fa_read_u32(FILE *fp)
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

fa_wavfmt_t fa_wavfmt_readheader(FILE *fp)
{
	long            nskip, x_size;
    unsigned short  format;
	unsigned short  channels, block_align;
	unsigned long   samplerate;
	unsigned long   bits_per_sample;
	unsigned long   bytes_per_sample;
	unsigned long   data_size;
	unsigned char   temp[SIZE_ID];
	fa_wavfmt_t     fmt;

	fread(temp, sizeof(*temp), SIZE_ID, fp);

	if (memcmp(temp, "RIFF", (size_t)SIZE_ID)!=0) {
		fprintf(stderr, "file is not WAVE format!\n");
		exit(0);
	}
	fread(temp, sizeof(*temp), SIZE_LONG, fp);

	fread(temp, sizeof(*temp), SIZE_ID, fp);
	if (memcmp(temp, "WAVE", (size_t)SIZE_ID)!=0) {
		fprintf(stderr, "file is not WAVE format!\n");
		exit(0);
	}
	fread(temp, sizeof(*temp), SIZE_ID, fp);
    /* skip chunks except for "fmt " or "data" */
	while (memcmp(temp, "fmt ", (size_t)SIZE_ID)!=0) {
		nskip = fa_read_u32(fp);
		if (nskip!=0) {
			fseek(fp, nskip, SEEK_CUR);
		}
	}

	x_size = fa_read_u32(fp);
	format = fa_read_u16(fp);
	x_size -= SIZE_SHORT;
	if (WAVE_FORMAT_PCM != format) {
		fprintf(stderr, "error! unsupported WAVE file format.\n");
		exit(0);
	}

	channels = fa_read_u16(fp);
	x_size -= SIZE_SHORT;

	samplerate = fa_read_u32(fp);
	x_size -= SIZE_LONG;

	fa_read_u32(fp);                                            /* skip bytes/s */
	block_align     = fa_read_u16(fp);                          /* skip block align */
	bits_per_sample = fa_read_u16(fp);                          /* bits/sample */
	bytes_per_sample= (bits_per_sample + BITS_PER_BYTE - 1)/BITS_PER_BYTE;
	block_align     = bytes_per_sample * channels;

	x_size -= SIZE_LONG + SIZE_SHORT + SIZE_SHORT;              /* skip additional part of "fmt " header */
	if (x_size!=0) {
		fseek(fp, x_size, SEEK_CUR);
	}

	/* skip chunks except for "data" */
	fread(temp, sizeof(*temp), SIZE_ID, fp);
	while (memcmp(temp, "data", SIZE_ID)!=0) {
		nskip = fa_read_u32(fp);                                /* read chunk size */
		fseek(fp, nskip, SEEK_CUR);
		fread(temp, sizeof(*temp), SIZE_ID, fp);
	}
	data_size = fa_read_u32(fp);

	fmt.format           = format;
	fmt.channels         = channels;
	fmt.samplerate       = samplerate;
	fmt.bytes_per_sample = bytes_per_sample;
	fmt.block_align      = block_align;
	fmt.data_size        = data_size / block_align;             /* byte to short */

	return fmt;
}


/*------------Write minimal Wave Header------------*/
/* total=44                                        */
/* 8                                               */
/* RIFF                                          4 */
/* chunk size                                    4 */
/*                                                 */
/* 12+16+8 = 36                                    */
/* 12                                              */
/* WAVE                                          4 */
/* fmt                                           4 */
/* chunk size (fmt chunk size is ever 16)        4 */
/*                                                 */
/* 16                                              */
/* format type                                   2 */
/* number of channels                            2 */
/* sampleing samplerate                          4 */
/* bytes per second                              4 */
/* block align per                               2 */
/* bits per sample                               2 */
/*                                                 */
/* 8                                               */
/* data                                          4 */
/* wav data chunk size in bytes(chunk size-36)   4 */

void fa_wavfmt_writeheader(fa_wavfmt_t fmt, FILE *fp)
{
	unsigned short  block_align;
	unsigned short  bits_per_sample;
	unsigned long   samples_per_sec;
	unsigned long   bytes_per_sec;
	unsigned long   fmt_header_size;

	samples_per_sec  = fmt.channels         * fmt.samplerate;
	bytes_per_sec    = samples_per_sec      * fmt.bytes_per_sample;
	block_align      = fmt.channels         * fmt.bytes_per_sample;
	bits_per_sample  = fmt.bytes_per_sample * BITS_PER_BYTE;
	fmt_header_size  = SIZE_SHORT * 4       + SIZE_LONG *2;

	fseek(fp, 0, SEEK_SET);
	fwrite("RIFF", sizeof(char), SIZE_ID         , fp);
	fa_write_u32(fmt.data_size*fmt.block_align+36, fp);
	fwrite("WAVE", sizeof(char), SIZE_ID         , fp);
	fwrite("fmt ", sizeof(char), SIZE_ID         , fp);
	fa_write_u32(fmt_header_size                 , fp);
	fa_write_u16(fmt.format                      , fp);
	fa_write_u16(fmt.channels                    , fp);
	fa_write_u32(fmt.samplerate                  , fp);
	fa_write_u32(bytes_per_sec                   , fp);
	fa_write_u16(block_align                     , fp);
	fa_write_u16(bits_per_sample                 , fp);
	fwrite("data", sizeof(char), SIZE_ID         , fp);
	fa_write_u32(fmt.data_size * fmt.block_align , fp);
}


