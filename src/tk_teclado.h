
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

#ifndef TECLADO_H
#define TECLADO_H

// Variáveis Externas
extern BYTE bKBOUT;
extern int ColarRapido;

// Protótipos
void KeybInicia(void);
void KeybAtualiza(DWORD totalcycles);
void KeybFinaliza(void);
void KeybUnset();
void KeybQueueKeypress (int key, BOOL down, BOOL ascii);
bool KeybIsPasting();

// Protótipos Softswitches
BYTE __stdcall KeybCTRL0(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall KeybCTRL1(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall KeybKBOUT(WORD, BYTE, BYTE, BYTE);
BYTE __stdcall KeybKBIN (WORD, BYTE, BYTE, BYTE);

#endif
// EOF