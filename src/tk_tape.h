
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

#ifndef TAPE_H
#define TAPE_H

// Definições
#define TB_REW  0x01
#define TB_STOP	0x02

#define CT2_MAGIC "CTK2"
#define CT2_CAB_A "CA"
#define CT2_CAB_B "CB"
#define CT2_DADOS "DA"

#include <pshpack1.h>

typedef struct STKCab
{
	BYTE Nome[6];
	BYTE TotalBlocos;
	BYTE BlocoAtual;
} TTKCab;

typedef struct STKEnd
{
	WORD EndInicial;
	WORD EndFinal;
} TTKEnd;

typedef struct SCh
{
	BYTE ID[2];
	WORD Tam;
} TCh;

#include <poppack.h>

// Protótipos
void TapeFinaliza();
void TapeInicializa();
void TapeAtualiza(DWORD);
void TapeSelect();
int  TapeInsere(char *, int, int);
void TapeRemove();
void TapeBotao(int);
void TapeNotifyInvalidImage (char*, int);
char *TapeNomeImagem();
void TapePegaStatusLed(int *);

// Protótipos Softswitches
BYTE __stdcall TapeMOTOR(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall TapeCASOUT(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall TapeCASIN(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall TapeCASIN2(WORD, BYTE, BYTE, BYTE);

#endif // TAPE_H

// EOF