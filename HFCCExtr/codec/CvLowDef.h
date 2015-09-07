/****************************************************************************
*                                                                           *
* CvDef.h -- USER low type define       									*
*                                                                           *
* Civet OS 3.1.0.1															*
*                                                                           *
* Copyright (c) 1999-2001, UDC Corp. All rights reserved.                   *
* All type define can be modified here                                      *
* Version 0.1                                                               *
*                                                                           *
****************************************************************************/
#ifndef	_CIVET_OS_LOW_DEFINE_3_1_0_1_H
#define	_CIVET_OS_LOW_DEFINE_3_1_0_1_H
#include "String.h"

#ifndef NULL
#define NULL				((void *)0)
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef	FAR
#define		FAR
#define		NEAR
#endif
typedef	char				COS_CHAR;
typedef	short				COS_SHORT;
typedef	long				COS_LONG;
typedef	int					COS_INT;
typedef	unsigned char		COS_UCHAR;
typedef	unsigned short		COS_USHORT;
typedef	unsigned long		COS_ULONG;
typedef	unsigned int		COS_UINT;
typedef	COS_SHORT			COS_BOOL;
typedef	COS_USHORT			COS_TEXT;
#define	COS_FLOAT			float
#define	COS_DOUBLE			double

typedef	COS_CHAR*			COS_PCHAR;
typedef	COS_SHORT*			COS_PSHORT;
typedef	COS_LONG*			COS_PLONG;
typedef	COS_INT*			COS_PINT;
typedef	COS_TEXT*			COS_PTEXT;
typedef	COS_BOOL*			COS_PBOOL;
typedef	COS_FLOAT*			COS_PFLOAT;
typedef	COS_DOUBLE*			COS_PDOUBLE;

typedef	COS_CHAR	FAR*	COS_LPCHAR;
typedef	COS_SHORT	FAR*	COS_LPSHORT;
typedef	COS_LONG	FAR*	COS_LPLONG;
typedef	COS_INT		FAR*	COS_LPINT;
typedef	COS_TEXT	FAR*	COS_LPTEXT;
typedef	COS_BOOL	FAR*	COS_LPBOOL;
typedef	COS_FLOAT	FAR*	COS_LPFLOAT;

typedef	COS_UCHAR*			COS_PUCHAR;
typedef	COS_USHORT*			COS_PUSHORT;
typedef	COS_ULONG*			COS_PULONG;
typedef	COS_UINT*			COS_PUINT;

typedef	COS_UCHAR	FAR*	COS_LPUCHAR;
typedef	COS_USHORT	FAR*	COS_LPUSHORT;
typedef	COS_ULONG	FAR*	COS_LPULONG;
typedef	COS_UINT	FAR*	COS_LPUINT;

typedef unsigned char		COS_BYTE;
typedef unsigned short		COS_WORD;
typedef unsigned long		COS_DWORD;

typedef unsigned char*		COS_PBYTE;
typedef unsigned short*		COS_PWORD;
typedef unsigned long*		COS_PDWORD;

#define	COS_WPARAM			COS_DWORD
#define	COS_LPARAM			COS_LONG

typedef COS_DWORD			COS_LPSTR;
typedef COS_DWORD			COS_LPCSTR;
typedef COS_DWORD			COS_LRESULT;

typedef	void				COS_VOID;
typedef void*				COS_PVOID;
typedef void	FAR*		COS_LPVOID;

typedef	COS_DWORD			COS_COLORREF;

#define	COS_CONST			const
#define	COS_STATIC			static
#define	COS_STATUS			COS_DWORD
#define	COS_HANDLE			COS_DWORD


#define COS_MAKEWORD(a, b)	((COS_WORD)(((COS_BYTE)(a)) | ((COS_WORD)((COS_BYTE)(b))) << 8))
#define COS_MAKELONG(a, b)	((COS_LONG)(((COS_WORD)(a)) | ((COS_DWORD)((COS_WORD)(b))) << 16))
#define COS_LOWORD(l)		((COS_WORD)(l))
#define COS_HIWORD(l)		((COS_WORD)(((COS_DWORD)(l) >> 16) & 0xFFFF))
#define COS_LOBYTE(w)		((COS_BYTE)(w))
#define COS_HIBYTE(w)		((COS_BYTE)(((COS_WORD)(w) >> 8) & 0xFF))
#define	COS_MIDDLE(x,y)		((COS_SHORT)((x+y)>>1))

#define COS_RGB(r,g,b)		((COS_COLORREF)(((COS_BYTE)(r)|((COS_WORD)((COS_BYTE)(g))<<8))|(((COS_DWORD)(COS_BYTE)(b))<<16)))
#define	COS_ROUNDDWORD(fig)	((((COS_DWORD)fig+sizeof(COS_DWORD)-1)/sizeof(COS_DWORD) )*sizeof(COS_DWORD))

#endif//_CIVET_OS_LOW_DEFINE_3_1_0_1_H