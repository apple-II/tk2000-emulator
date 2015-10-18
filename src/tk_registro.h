
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

#ifndef REGISTRO_H
#define REGISTRO_H

// Definições do registro
#define POSX         "Posicao X Janela"
#define POSY         "Posicao Y Janela"
#define PREFERENCIAS "Preferencias"
#define DIRINIC      "Diretorio Inicio"
#define VERSAO       "Versao"
#define CONFIG       "Configuracao"
#define RUNOS        "Sistema Op"
#define EMUVIDEO     "Emulacao Video"
#define EMUVIDEOM    "Video Monocromatica"
#define EMUJOY       "Emulacao Joystick"
#define EMUSOM       "Emulacao Som"
#define EMUSLOT      "Slot Expansao"
#define EMUVEL       "Velocidade"
#define EMUVELCPU    "Velocidade Emulacao"
#define EMUDISCO     "Velocidade Disco Rapida"
#define EMUSCANLINES "Scan Lines" // Scan Lines
#define EMUIMPRESSORA "Impressora"
#define EMUCOLARRAPIDO "Colar Rapido"
#define ULTIMPEND    "Ultimo End Importacao"
#define ULTEXPENDI   "Ultimo End Inic Exportacao"
#define ULTEXPENDF   "Ultimo End Final Exportacao"

// Protótipos
int  RegLoadString (char*, char*, int, char*, DWORD);
int  RegLoadValue  (char*, char*, int, DWORD *);
void RegSaveString (char*, char*, int, char*);
void RegSaveValue  (char*, char*, int, DWORD);

#endif
// EOF