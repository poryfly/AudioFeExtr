#ifndef _CYGWIN_INTTYPES_H
#define _CYGWIN_INTTYPES_H

//#define DEBUG_TIME
#define MALLOC_NEEDED  
//#define WIN32
/* /usr/include/inttypes.h for CYGWIN
 * Copyleft 2001-2002 by Felix Buenemann
 * <atmosfear at users.sourceforge.net>
 *
 * Should be installed into /usr/include
 * as inttypes.h
 */
typedef char *  caddr_t;
typedef char int8_t;
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;

#ifdef  WIN32
typedef unsigned __int64  u_int64_t;
#else
typedef long long __int64; 
typedef unsigned long long  u_int64_t;
#endif


typedef __int64 int64_t;
typedef int32_t register_t;
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;
#endif /* _CYGWIN_INTTYPES_H */

