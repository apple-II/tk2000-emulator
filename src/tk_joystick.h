
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

#ifndef JOYSTICK_H
#define JOYSTICK_H

// Variáveis Externas
extern DWORD	  joytype;

// Protótipos
void JoyInitialize();
BOOL JoyProcessKey(int,BOOL,BOOL,BOOL);
void JoyReset();
void JoySetButton(int,BOOL);
BOOL JoySetEmulationType (HWND,DWORD);
void JoySetPosition(int,int,int,int);
void JoyUpdatePosition (DWORD);
BOOL JoyUsingMouse();
BYTE JoyVerificaTecla(int);


// Protótipos Softswitches

#endif
// EOF