/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi (Chengdu, China)

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


  filename: fa_getopt.h 
  version : v1.0.0
  time    : 2012/07/08 01:01 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )

  comment : this file is the simple template which will be used in falab,
            it will be changed according to the project.

*/

#ifndef _FA_GETOPT_H
#define _FA_GETOPT_H

#include "fa_fir.h"

/*
    Below macro maybe defined in fa_print, if you do not want to use fa_print,
    you can define any other printf functions which you like.  Moreover, if you
    dont want print anything (for the effienece purpose of the program), you 
    can also define null, and the program will print nothing when running.
*/
#ifndef FA_PRINT 
//#define FA_PRINT(...)       
#define FA_PRINT       printf
#define FA_PRINT_ERR   FA_PRINT
#define FA_PRINT_DBG   FA_PRINT
#endif

#define DOWNSAMPLE 8000

enum{
    FA_DECIMATE = 0,
    FA_INTERP,
    FA_RESAMPLE,
};

extern char  opt_inputfile[] ;
extern char  opt_outputfile[];

extern int   opt_type      ;
extern int   opt_downfactor;
extern int   opt_upfactor  ;
extern float opt_gain      ;


int fa_parseopt(int argc, char *argv[]);

#endif
