#ifndef	_WAVE_FORMAT
#define	_WAVE_FORMAT

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct waveFormat               
{
	long format;
	unsigned long frequency;
	unsigned short channels;
	unsigned short bytes_per_sample;
	unsigned short block_align;
	unsigned long data_size;
//	unsigned long  head_size;//jhy 08-9-21
} waveFormat;


#define WAVE_FORMAT_UNKNOWN             (0x0000)
#define WAVE_FORMAT_PCM2                 (0x0001)
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

#define SIZE_ID       4
#define SIZE_LONG     4
#define SIZE_SHORT    2
#define BITS_PER_BYTE 8


waveFormat readWaveHeader(FILE *fp);
void writeWaveHeader(waveFormat fmt, FILE *fp);



#ifdef __cplusplus
}
#endif










#endif//_WAVE_FORMAT
