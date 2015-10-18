
/*  Emulador do computador TK3000 //e (Microdigital)
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

#ifndef IMPRESSORA_H
#define IMPRESSORA_H

// Variáveis Externas
extern BYTE ImpressoraPorta;

// Protótipos
void ImpressoraInicializa();
void ImpressoraFinaliza();
void ImpressoraAtualiza(DWORD);
void ImpressoraDefinePorta(int);
void ImpressoraReset();
void ImpressoraMudaAcentuado();
void ImpressoraEnviaChar(BYTE);

// Protótipos Softswitches
BYTE __stdcall ImpressoraStrobe(WORD, BYTE, BYTE, BYTE);

#endif
// EOF