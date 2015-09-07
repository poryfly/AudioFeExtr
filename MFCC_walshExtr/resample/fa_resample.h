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


  filename: fa_resample.h 
  version : v1.0.0
  time    : 2012/07/12 20:42 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  Note    : this source code is written by the reference article 
            [
              Interploation and Decimation of Digital Signals - A Tutorial Review
                 PROCEEDINGS OF THE IEEE, VOL.69, NO.3, MARCH 1981        
                 RONALD E.CROCHIERE, senior member, IEEE, AND LAWRENCE R.RABINER, fellow, IEEE
            ]

*/



#ifndef _RESAMPLE_H
#define _RESAMPLE_H

#include "fa_fir.h"

#ifdef __cplusplus 
extern "C"
{ 
#endif  


typedef unsigned uintptr_t;

/*#define FA_DEFAULT_FRAMELEN     1024  */          /* default num of sample in */
#define FA_DEFAULT_FRAMELEN     1152  
#define FA_FRAMELEN_MAX         (160*147+8192)  /* 8192 no meaning, I just set here to safe regarding */

#define FA_RATIO_MAX            16

uintptr_t fa_decimate_init(int M, float gain, win_t win_type);
void      fa_decimate_uninit(uintptr_t handle);

uintptr_t fa_interp_init(int L, float gain, win_t win_type);
void      fa_interp_uninit(uintptr_t handle);

uintptr_t fa_resample_filter_init(int L, int M, float gain, win_t win_type);
void      fa_resample_filter_uninit(uintptr_t handle);

int fa_get_resample_framelen_bytes(uintptr_t handle);
int fa_decimate(uintptr_t handle, unsigned char *sample_in, int sample_in_size,
                                  unsigned char *sample_out, int *sample_out_size);
int fa_interp(uintptr_t handle, unsigned char *sample_in, int sample_in_size,
                                unsigned char *sample_out, int *sample_out_size);
int fa_resample(uintptr_t handle, unsigned char *sample_in, int sample_in_size,
                                  unsigned char *sample_out, int *sample_out_size);


#ifdef __cplusplus 
}
#endif  


#endif
