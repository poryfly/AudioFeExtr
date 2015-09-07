#include "mpegaudio.h"
#include "stddef.h"
#include "string.h"
#include "CvLowDef.h"

#define CONFIG_MPEGAUDIO_HP         //MPadecoder-vc-05-02-20-v0.8S-lp-hp

#define HEADER_SIZE 4
#define BACKSTEP_SIZE 512

#ifdef CONFIG_MPEGAUDIO_HP
#define USE_HIGHPRECISION
#endif

#ifdef USE_HIGHPRECISION
#define FRAC_BITS   23   /* fractional bits for sb_samples and dct */
#define WFRAC_BITS  16   /* fractional bits for window */
#else
#define FRAC_BITS   15   /* fractional bits for sb_samples and dct */
#define WFRAC_BITS  14   /* fractional bits for window */
#endif

#define FRAC_ONE    (1 << FRAC_BITS)

#define MULL(a,b) (((int64_t)(a) * (int64_t)(b)) >> FRAC_BITS)
#define MUL64(a,b) ((int64_t)(a) * (int64_t)(b))
#define FIX(a)   ((int)((a) * FRAC_ONE))
/* WARNING: only correct for posititive numbers */
#define FIXR(a)   ((int)((a) * FRAC_ONE + 0.5))
#define FRAC_RND(a) (((a) + (FRAC_ONE/2)) >> FRAC_BITS)

#if FRAC_BITS <= 15
typedef int16_t MPA_INT;
#else
typedef int32_t MPA_INT;
#endif

/****************/


#define M_PI    3.14159265358979323846     //lizhen

struct GranuleDef;

typedef struct _MPADecodeContext {
    uint8_t inbuf1[2][MPA_MAX_CODED_FRAME_SIZE + BACKSTEP_SIZE];	/* input buffer */
    int inbuf_index;
    uint8_t *inbuf_ptr, *inbuf;
    unsigned int frame_size;
    int free_format_frame_size; /* frame size in case of free format
                                   (zero if currently unknown) */
    /* next header (used in free format parsing) */
    uint32_t free_format_next_header; 
    int error_protection;
    int layer;
    int sample_rate;
    int sample_rate_index; /* between 0 and 8 */
    int bit_rate;
    int old_frame_size;
    GetBitContext gb;
    int nb_channels;
    int mode;
    int mode_ext;
    int lsf;
    MPA_INT synth_buf[MPA_MAX_CHANNELS][512 * 2] ;//__attribute__((aligned(16)));
    int synth_buf_offset[MPA_MAX_CHANNELS];
    int32_t sb_samples[MPA_MAX_CHANNELS][36][SBLIMIT] ;//__attribute__((aligned(16)));
    int32_t mdct_buf[MPA_MAX_CHANNELS][SBLIMIT * 18]; /* previous samples, for layer 3 MDCT */
//#ifdef CODEC_DEBUG
    int frame_count;
//#endif
    void (*compute_antialias)(struct _MPADecodeContext *s, struct GranuleDef *g);
} MPADecodeContext;

typedef struct
{
     unsigned long   sample_rate;
     unsigned long   bit_rate;
	 unsigned long   mode;
     unsigned long   frame_size;
}codingstate;

/* layer 3 "granule" */
typedef struct GranuleDef {
    uint8_t scfsi;
    int part2_3_length;
    int big_values;
    int global_gain;
    int scalefac_compress;
    uint8_t block_type;
    uint8_t switch_point;
    int table_select[3];
    int subblock_gain[3];
    uint8_t scalefac_scale;
    uint8_t count1table_select;
    int region_size[3]; /* number of huffman codes in each region */
    int preflag;
    int short_start, long_end; /* long/short band indexes */
    uint8_t scale_factors[40];
    int32_t sb_hybrid[SBLIMIT * 18]; /* 576 samples */
} GranuleDef;

typedef struct WavbufData {
	int fft_size;
	int step;
	int BufNum;
	int samplerate;
	int channel;
	int processtimes;
	short LeftBuff[BACKSTEP_SIZE*2];
	int LeftSize;
	int *frequences;
	int forsearch;
	int targf;
	double distunit;
	int FrameLen;
	COS_HANDLE mpa_dec_handle;
}WavbufData;

#define MODE_EXT_MS_STEREO 2
#define MODE_EXT_I_STEREO  1

/* layer 3 huffman tables */
typedef struct HuffTable {
    int xsize;
    const uint8_t *bits;
    const uint16_t *codes;
} HuffTable;
