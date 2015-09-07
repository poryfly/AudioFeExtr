#ifndef _MYOPTIMIZEH_
#define _MYOPTIMIZEH_

#define CONFIG_MPEGAUDIO_HP

#ifdef CONFIG_MPEGAUDIO_HP
unsigned int norm_table_hp[15] = {
 11184810,
 9586980,
 8947848,
 8659208,
 8521760,
 8454660,
 8421504,
 8405024,
 8396808,
 8392706,
 8390656,
 8389632,
 8389120,
 8388864,
 8388736
 };
#else
unsigned int norm_table_lp[15] = {
   43690,
   37449,
   34952,
   33825,
   33288,
   33026,
   32896,
   32832,
   32800,
   32784,
   32776,
   32772,
   32770,
   32769,
   32768
};
#endif

#endif