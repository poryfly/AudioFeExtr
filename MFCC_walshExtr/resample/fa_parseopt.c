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


  filename: fa_getopt.c 
  version : v1.0.0
  time    : 2012/07/08 18:42 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__ 
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "fa_parseopt.h"
#include "fa_resample.h"

/*global option vaule*/
char  opt_inputfile[256]  = "";
char  opt_outputfile[256] = "";
int   opt_type      = FA_DECIMATE;
/*default resample is 48kHz->44.1kHz*/
// int   opt_downfactor= 147;//160;
// int   opt_upfactor  = 160;//147;
// float opt_gain      = 1.0;
int   opt_downfactor= 6;//160;
int   opt_upfactor  = 1;//147;
float opt_gain      = 1.0;

static int input_downfactor = 0;
static int input_upfactor   = 0;

const char *usage =
"\n\n"
"Usage: faresample <-i> <inputfile> <-o> <outputfile> [options] \n"
"\n\n"
"See also:\n"
"    --help               for short help on ...\n"
"    --long-help          for a description of all options for ...\n"
"    --license            for the license terms for falab.\n\n";

const char *default_set =
"\n\n"
"No argument input, run by default settings\n"
"    --type       [resample]\n"
"    --downfactor [160]\n"
"    --upfactor   [147]\n"
"    --gain       [1.0]\n"
"\n\n";

const char *short_help =
"\n\n"
"Usage: faresample <-i> <inputfile> <-o> <outputfile> [options] ...\n"
"Options:\n"
"    -i <inputfile>       Set input filename\n"
"    -o <outputfile>      Set output filename\n"
"    -t <filtertype>      Set decimiate/interp/resample type\n"
"    -d <downfactor>      Set downfactor\n"
"    -u <upfactor>        Set downfactor\n"
"    -g <gain>            Set gain\n"
"    --help               Show this abbreviated help.\n"
"    --long-help          Show complete help.\n"
"    --license            for the license terms for falab.\n"
"\n\n";

const char *long_help =
"\n\n"
"Usage: testwavfmt <-i> <inputfile> <-o> <outputfile> [options] ...\n"
"Options:\n"
"    -i <inputfile>       Set input filename\n"
"    -o <outputfile>      Set output filename\n"
"    --help               Show this abbreviated help.\n"
"    --long-help          Show complete help.\n"
"    --license            for the license terms for falab.\n"
"    --input <inputfile>  Set input filename\n"
"    --output <inputfile> Set input filename\n"
"    --type <filtertype>  Set decimiate/interp/resample type\n"
"    --down <df>          Set downfactor\n"
"    --up   <uf>          Set upfactor\n"
"    --gain <gain>        Set gain\n"
"\n\n";

const char *license =
"\n\n"
"**************************************  WARN  *******************************************\n"
"*    Please note that the use of this software may require the payment of patent        *\n"
"*    royalties. You need to consider this issue before you start building derivative    *\n"
"*    works. We are not warranting or indemnifying you in any way for patent royalities! *\n"
"*                                                                                       *\n" 
"*                YOU ARE SOLELY RESPONSIBLE FOR YOUR OWN ACTIONS!                       *\n"
"*****************************************************************************************\n"
"\n"
"\n"
"falab - free algorithm lab \n"
"Copyright (C) 2012 luolongzhi (Chengdu, China)\n"
"\n"
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
"\n"
"\n"
"falab src code is based on the signal processing theroy, optimize theroy, ISO ref, etc.\n"
"The purpose of falab is building a free algorithm lab to help people to learn\n"
"and study algorithm, exchanging experience.    ---luolongzhi 2012.07.08"
"\n";

static void fa_printopt()
{
    FA_PRINT("NOTE: configuration is below\n");
    FA_PRINT("NOTE: inputfile = %s\n", opt_inputfile);
    FA_PRINT("NOTE: outputfile= %s\n", opt_outputfile);
    FA_PRINT("NOTE: type      = %d\n", opt_type);
    FA_PRINT("      (0:decimiate; 1:interp; 2:resample)\n");
    FA_PRINT("NOTE: down      = %d\n", opt_downfactor);
    FA_PRINT("NOTE: up        = %d\n", opt_upfactor);
    FA_PRINT("NOTE: gain      = %f\n", opt_gain);
}

/**
 * @brief: check the input value valid, should check the valid scope
 *
 * @return: 0 if success, -1 if error
 */
static int fa_checkopt(int argc)
{
    float ratio;

    if(argc < 5) {
        FA_PRINT_ERR("FAIL: input and output file should input\n");
        return -1;
    }

    if(strlen(opt_inputfile) == 0 || strlen(opt_outputfile) == 0) {
        FA_PRINT_ERR("FAIL: input and output file should input\n");
        return -1;
    }

    if((input_upfactor) && (!input_downfactor))
        opt_downfactor = 1;

    if((!input_upfactor) && (input_downfactor))
        opt_upfactor = 1;

    ratio = ((float)opt_upfactor)/opt_downfactor;
    if((ratio      > FA_RATIO_MAX) || 
       ((1./ratio) > FA_RATIO_MAX)) {
        FA_PRINT_ERR("FAIL: ratio not support, you can use cascade method to implement \n");
        return -1;
    }

    FA_PRINT("SUCC: check option ok\n");
    return 0;
}


/**
 * @brief: parse the command line
 *         this is the simple template which will be used by falab projects
 *
 * @param:argc
 * @param:argv[]
 *
 * @return: 0 if success, -1 if error(input value maybe not right)
 */
int fa_parseopt(int argc, char *argv[])
{
    int ret;
    const char *die_msg = NULL;

    while (1) {
        static char * const     short_options = "hHLi:o:t:d:u:g:";  
        static struct option    long_options[] = 
                                {
                                    { "help"       , 0, 0, 'h'}, 
                                    { "long-help"  , 0, 0, 'H'},
                                    { "license"    , 0, 0, 'L'},
                                    { "input"      , 1, 0, 'i'},                 
                                    { "output"     , 1, 0, 'o'},                 
                                    { "type"       , 1, 0, 't'},        
                                    { "down"       , 1, 0, 'd'},
                                    { "up"         , 1, 0, 'u'},
                                    { "gain"       , 1, 0, 'g'},
                                    {0             , 0, 0,  0},
                                };
        int c = -1;
        int option_index = 0;

        c = getopt_long(argc, argv, 
                        short_options, long_options, &option_index);

        if (c == -1) {
            break;
        }

        if (!c) {
            die_msg = usage;
            break;
        }

        switch (c) {
            case 'h': {
                          die_msg = short_help;
                          break;
                      }

            case 'H': {
                          die_msg = long_help;
                          break;
                      }
                      
            case 'L': {
                          die_msg = license;
                          break;
                      }

            case 'i': {
                          if (sscanf(optarg, "%s", opt_inputfile) > 0) {
                              FA_PRINT("SUCC: inputfile is %s\n", opt_inputfile);
                          }else {
                              FA_PRINT_ERR("FAIL: no inputfile\n");
                          }
                          break;
                      }

            case 'o': {
                          if (sscanf(optarg, "%s", opt_outputfile) > 0) {
                              FA_PRINT("SUCC: outputfile is %s\n", opt_outputfile);
                          }else {
                              FA_PRINT_ERR("FAIL: no outputfile\n");
                          }
                          break;
                      }

            case 't': {
                          unsigned int i;
                          if (sscanf(optarg, "%u", &i) > 0) {
                              opt_type = i;
                              FA_PRINT("SUCC: set type = %u\n", opt_type);
                          }
                          break;
                      }

            case 'd': {
                          unsigned int i;
                          if (sscanf(optarg, "%u", &i) > 0) {
                              opt_downfactor = i;
                              input_downfactor = 1;
                              FA_PRINT("SUCC: set downfactor= %u\n", opt_downfactor);
                          }
                          break;
                      }

            case 'u': {
                          unsigned int i;
                          if (sscanf(optarg, "%u", &i) > 0) {
                              opt_upfactor = i;
                              input_upfactor = 1;
                              FA_PRINT("SUCC: set upfactor= %u\n", opt_upfactor);
                          }
                          break;
                      }

            case 'g': {
                          float i;
                          if (sscanf(optarg, "%f", &i) > 0) {
                              opt_gain = i;
                              FA_PRINT("SUCC: set gain = %f\n", opt_gain);
                          }
                          break;
                      }

            case '?':
            default:
                      die_msg = usage;
                      break;
        }
    }

    if(die_msg) {
        FA_PRINT("%s\n", die_msg);
        goto fail;
    }

    /*check the input validity*/
    ret = fa_checkopt(argc);
    if(ret) {
        die_msg = usage;
        FA_PRINT("%s\n", die_msg);
        goto fail;
    }

    /*print the settings*/
    fa_printopt();

    return 0;
fail:
    return -1;
}
