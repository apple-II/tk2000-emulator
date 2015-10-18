
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

#ifndef VIDEO_H
#define VIDEO_H

// Definições
enum VIDEOTYPE
{
	VT_MONO=0,
	VT_COLOR_STANDARD,
	VT_COLOR_TEXT_OPTIMIZED,
	VT_COLOR_TVEMU,
	VT_NUM_MODES
};

// Variáveis Externas
extern BOOL		behind;
extern DWORD	emulmsec;
extern BOOL		graphicsmode;	
extern DWORD	monochrome;
extern DWORD	videotype;
extern DWORD	vidmode;
extern BOOL		behind;
extern DWORD	emulmsec;
extern DWORD	emulmsec_frac;
extern int		ScanLines;


// Protótipos
BOOL	VideoApparentlyDirty ();
void	VideoCheckPage (BOOL);
void	VideoChooseColor ();
void	VideoDestroy ();
void	VideoDisplayLogo ();
BOOL	VideoHasRefreshed ();
void	VideoInitialize	();
void	VideoRealizePalette (HDC);
void	VideoRedrawScreen ();
void	VideoRefreshScreen ();
void	VideoReinitialize ();
void	VideoResetState	();
//void	VideoUpdateVbl (DWORD,BOOL);
void	CreateColorMixMap();

// Protótipos Softswitches
BYTE __stdcall VideoCheckMode (WORD,BYTE,BYTE,BYTE);
BYTE __stdcall VideoCheckVbl (WORD,BYTE,BYTE,BYTE);
BYTE __stdcall VideoSetMode (WORD,BYTE,BYTE,BYTE);

#endif
// EOF