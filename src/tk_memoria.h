
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

#ifndef MEMORIA_H
#define MEMORIA_H

// Definições Gerais
BYTE __stdcall NullIo (WORD programcounter, BYTE address, BYTE write, BYTE value);

// Definições de Tipos
typedef	BYTE (__stdcall	*iofunction)(WORD,BYTE,BYTE,BYTE);

// Variáveis Externas
extern iofunction ioread[0x100];
extern iofunction iowrite[0x100];
//extern BYTE* memaux;
extern BYTE* memdirty;
extern BYTE* memmain;
extern BYTE* memread [0x100];
extern BYTE* memwrite[0x100];
extern BYTE	 SlotAux;
extern BYTE  NewSlotAux;

// Protótipos
int   MemAtualizaSlotAux(void);
int   MemRetiraSlotAux();
int   MemInsereSlotAux(iofunction[0x10], iofunction[0x10],char*);
BYTE  mem_readb(WORD, BOOL);
void  mem_writeb(WORD, BYTE);
WORD  mem_readw(WORD, BOOL);
void  mem_writew(WORD, WORD);
void  MemDestroy();
BYTE* MemGetMainPtr(WORD);
void  MemInitialize();
void  MemReset();
BYTE  MemReturnRandomData(BYTE);
BOOL  MemImportar(WORD);
BOOL  MemExportar(WORD,WORD);
void  MemResetPaging();

// Protótipos Softswitches
BYTE __stdcall MemCheckPaging (WORD,BYTE,BYTE,BYTE);
BYTE __stdcall MemSetPaging (WORD,BYTE,BYTE,BYTE);

#endif
// EOF