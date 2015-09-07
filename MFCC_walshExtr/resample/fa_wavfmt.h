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


  filename: fa_wavfmt.h 
  version : v1.0.0
  time    : 2012/07/08 18:33 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  comment : only support pcm data now

*/


#ifndef	_FA_WAVFMT_H
#define	_FA_WAVFMT_H

#include <stdio.h>

#ifdef __cplusplus 
extern "C"
{ 
#endif  


typedef struct _fa_wavfmt_t
{
	unsigned short  format;

	unsigned short  channels;

	unsigned long   samplerate;

	unsigned short  bytes_per_sample;

	unsigned short  block_align;

	unsigned long   data_size;

}fa_wavfmt_t;


fa_wavfmt_t  fa_wavfmt_readheader (FILE *fp);
void         fa_wavfmt_writeheader(fa_wavfmt_t fmt, FILE *fp);

#ifdef __cplusplus 
}
#endif  



#endif
