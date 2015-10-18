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

#ifndef CPU_H
#define CPU_H

#include "tk_stdhdr.h"

typedef	struct _regsrec {
	BYTE a;	  // accumulator
	BYTE x;	  // index X
	BYTE y;	  // index Y
	BYTE ps;  // processor status
	WORD pc;  // program counter
	WORD sp;  // stack pointer
} regsrec, *regsptr;

// Variáveis Externas
extern regsrec	regs;

#ifdef CPUDEBUG
extern FILE *arquivocpu;
#endif

// Protótipos
DWORD CpuExecute(DWORD);
void  CpuInitialize();
void  CpuReinitialize();
void  CpuDestroy();
void  CpuIRQ();
void  CpuNMI();

#endif //CPU_H

// EOF