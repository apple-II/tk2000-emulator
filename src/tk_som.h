
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

#ifndef SOM_H
#define SOM_H

// Variáveis Externas
extern DWORD  soundtype;
extern double g_fClksPerSpkrSample;

// Protótipos
void SpkrDestroy ();
void SpkrInitialize ();
void SpkrReinitialize ();
void SpkrReset();
void SetClksPerSpkrSample();
BOOL SpkrSetEmulationType (HWND,DWORD);
static void InitRemainderBuffer(UINT nCyclesRemaining);
static void UpdateRemainderBuffer(LONG* pnCycleDiff);

void SpkrUpdate (DWORD);

// Protótipos Softswitches
BYTE __stdcall SpkrToggle (WORD,BYTE,BYTE,BYTE);

#endif
// EOF