
/*  Emulador do computador TK2000 (Microdigital)
 *  por Fábio Belavenuto - Copyright (C) 2004
 *
 *  Adaptado do emulador Applewin por Michael O'Brien
 *  Part of code is Copyright (C) 2003-2004 Tom Charlesworth
 *
 *  Este arquivo é distribuido pela Licença Pública Geral GNU.
 *  Veja o arquivo Licenca.txt distribuido com este software.
 *
 *  ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 *
 */

#undef CPUDEBUG
#undef USAR_HELP
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4995)
#pragma warning(disable: 4996)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WINDOWS
#include <dsound.h>
#include <dshow.h>
#include <windows.h>
#include <commctrl.h>
#include <ddraw.h>
#endif // _WINDOWS

#ifdef USAR_HELP
#include <htmlhelp.h>
#endif

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

#ifndef MAKEWORD
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif
#ifndef MAKELONG
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#endif
#ifndef LOWORD
#define LOWORD(l)           ((WORD)(l))
#endif
#ifndef HIWORD
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOBYTE
#define LOBYTE(w)           ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

// EOF