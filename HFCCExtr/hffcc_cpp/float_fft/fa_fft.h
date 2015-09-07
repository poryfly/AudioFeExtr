/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙�?(Chengdu, China)

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


  filename: fa_fft.h 
  version : v1.0.0
  time    : 2012/07/15 14:14 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#ifndef _FA_FFT_H
#define _FA_FFT_H

#ifdef __cplusplus 
extern "C"
{ 
#endif  


typedef unsigned uintptr_t;

uintptr_t fa_fft_init(int size);
void fa_fft_uninit(uintptr_t handle);

void fa_fft(uintptr_t handle, float *data);
void fa_ifft(uintptr_t handle, float* data);

#ifdef __cplusplus 
}
#endif  


#endif
