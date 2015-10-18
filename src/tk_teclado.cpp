/****************************************************************************
*
*  APPLE //E EMULATOR FOR WINDOWS                    
*
*  Copyright (C) 1994-96, Michael O'Brien.  All rights reserved.
*
***/

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

// Emula o teclado do TK2000

#include "tk_stdhdr.h"
#include "tk_teclado.h"
#include "tk_main.h"
#include "tk_cpu.h"
#include "tk_memoria.h"
#include "tk_janela.h"
#include "tk_video.h"
#include "tk_joystick.h"
#include "tk_tape.h"
#include "tk_impressora.h"
#include "tk_disco.h"

// Definições
#define VK_IGUALMAIS  0xBB
#define VK_VIRGULA    0xBC
#define VK_MENOS      0xBD
#define VK_PONTO      0xBE
#define VK_DOISPONTOS 0xBF
#define VK_APOSTROFO  0xC0
#define VK_PERGUNTA   0xC1
#define VK_NPPONTO    0xC2

#define FLGINVSH      0x01

typedef struct {
	BYTE linha;
	BYTE coluna;
	BYTE flags;
} TVk;

typedef struct {
	BYTE teclasub1;  // se shift off
	BYTE onshift1;   // se shift off
	BYTE teclasub2;  // se shift on
	BYTE onshift2;   // se shift on
} Convs;

const char caracs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 /?.>,<;:@^+=-*)('&!\"#$%\x0D";

// Variáveis
BYTE  bKBOUT  = 0;
int  ColarRapido = 0;

static bool g_bPasteFromClipboard = false;
static BYTE  bKBOUT_ant = 1;
static unsigned __int64 ultCicloP = 0, ultCicloL = 0;
static BYTE  bCTRL   = 0;
static bool  controlStatus = false;
static TVk   Teclas[256];
static Convs Conversoes[256];
static unsigned char matriz[8];				/* Matriz que mantém o status de cada tecla pressionada, já no formato da matriz do TK2000 */
static bool g_bClipboardActive = false;
static HGLOBAL hglb = NULL;
static LPTSTR lptstr = NULL;


/* Funções Internas */
//===========================================================================
static void ClipboardDone() {
	if (g_bClipboardActive) {
		g_bClipboardActive = false;
		GlobalUnlock(hglb);
		CloseClipboard();
	}
}

//===========================================================================
static void ClipboardInit() {
	ClipboardDone();

	if (!IsClipboardFormatAvailable(CF_TEXT))
		return;

	if (!OpenClipboard((HWND)framewindow))
		return;

	hglb = GetClipboardData(CF_TEXT);
	if (hglb == NULL) {	
		CloseClipboard();
		return;
	}

	lptstr = (char*) GlobalLock(hglb);
	if (lptstr == NULL) {	
		CloseClipboard();
		return;
	}

	g_bPasteFromClipboard = false;
	g_bClipboardActive = true;
}

//===========================================================================
static char ClipboardCurrChar(bool bIncPtr)
{
	char nKey;
	int nInc = 1;

	if((lptstr[0] == 0x0D) && (lptstr[1] == 0x0A)) {
		nKey = 0x0D;
		nInc = 2;
	} else {
		nKey = lptstr[0];
		if (nKey > 0x60 && nKey < 0x7B)
			nKey -= 0x20;
	}

	if(bIncPtr)
		lptstr += nInc;

	return nKey;
}

//===========================================================================
void ClipboardInitiatePaste() {
	if (g_bClipboardActive)
		return;

	g_bPasteFromClipboard = true;
}


//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void KeybUnset() {
	memset(matriz, 0, 8);
}

//===========================================================================
bool KeybIsPasting() {
	return g_bClipboardActive;
}

//===========================================================================
void KeybInicia(void)
{
	memset(Teclas, 0xFF, sizeof(TVk)*256);
	memset(Conversoes, 0, sizeof(Convs)*256);
	memset(matriz, 0, 8);

#define CONFTECLA(vlinha, vcoluna, vflags, tecla)	\
				Teclas[(tecla)].linha  = (vlinha);	\
				Teclas[(tecla)].coluna = (vcoluna);	\
				Teclas[(tecla)].flags  = (vflags);

	CONFTECLA(7,  5,  0,        '?')		// ?
	CONFTECLA(7,  5,  FLGINVSH, '/')		// SHIFT+?
	CONFTECLA(7,  4,  0,        '.')		// .
	CONFTECLA(7,  4,  FLGINVSH, '>')		// SHIFT+.
	CONFTECLA(7,  3,  0,        ',')		// ,
	CONFTECLA(7,  3,  FLGINVSH, '<')		// SHIFT+,
	CONFTECLA(7,  2,  0,        'M')
	CONFTECLA(7,  1,  0,        'N')
	CONFTECLA(7,  0,  0,        VK_RETURN)

	CONFTECLA(6,  5,  0,        ':')		// :
	CONFTECLA(6,  5,  FLGINVSH, ';')		// SHIFT+:
	CONFTECLA(6,  4,  0,        'L')
	CONFTECLA(6,  4,  FLGINVSH, '@')		// SHIFT+L
	CONFTECLA(6,  3,  0,        'K')
	CONFTECLA(6,  3,  FLGINVSH, '^')		// SHIFT+K
	CONFTECLA(6,  2,  0,        'J')
	CONFTECLA(6,  1,  0,        'H')
	CONFTECLA(6,  0,  0,        0x0B)		// UP 

	CONFTECLA(5,  5,  0,        'P')
	CONFTECLA(5,  5,  FLGINVSH, '+')		// SHIFT+P
	CONFTECLA(5,  4,  0,        'O')
	CONFTECLA(5,  4,  FLGINVSH, '=')		// SHIFT+O
	CONFTECLA(5,  3,  0,        'I')
	CONFTECLA(5,  3,  FLGINVSH, '-')		// SHIFT+I
	CONFTECLA(5,  2,  0,        'U')
	CONFTECLA(5,  1,  0,        'Y')
	CONFTECLA(5,  0,  0,        0x0A)		// DOWN

	CONFTECLA(4,  5,  0,        '0')
	CONFTECLA(4,  5,  FLGINVSH, '*')		// SHIFT+0
	CONFTECLA(4,  4,  0,        '9')
	CONFTECLA(4,  4,  FLGINVSH, ')')		// SHIFT+9
	CONFTECLA(4,  3,  0,        '8')
	CONFTECLA(4,  3,  FLGINVSH, '(')		// SHIFT+8
	CONFTECLA(4,  2,  0,        '7')
	CONFTECLA(4,  2,  FLGINVSH, '\'')		// SHIFT+7
	CONFTECLA(4,  1,  0,        '6')
	CONFTECLA(4,  1,  FLGINVSH, '&')		// SHIFT+6
	CONFTECLA(4,  0,  0,        0x15)		// RIGHT

	CONFTECLA(3,  5,  0,        '1')
	CONFTECLA(3,  5,  FLGINVSH, '!')		// SHIFT+1
	CONFTECLA(3,  4,  0,        '2')
	CONFTECLA(3,  4,  FLGINVSH, '"')		// SHIFT+2
	CONFTECLA(3,  3,  0,        '3')
	CONFTECLA(3,  3,  FLGINVSH, '#')		// SHIFT+3
	CONFTECLA(3,  2,  0,        '4')
	CONFTECLA(3,  2,  FLGINVSH, '$')		// SHIFT+4
	CONFTECLA(3,  1,  0,        '5')
	CONFTECLA(3,  1,  FLGINVSH, '%')		// SHIFT+5
	CONFTECLA(3,  0,  0,        VK_BACK)	// LEFT (Backspace)

	CONFTECLA(2,  5,  0,        'Q')
	CONFTECLA(2,  4,  0,        'W')
	CONFTECLA(2,  3,  0,        'E')
	CONFTECLA(2,  2,  0,        'R')
	CONFTECLA(2,  1,  0,        'T')
	CONFTECLA(2,  0,  0,        ' ')

	CONFTECLA(1,  5,  0,        'A')
	CONFTECLA(1,  4,  0,        'S')
	CONFTECLA(1,  3,  0,        'D')
	CONFTECLA(1,  2,  0,        'F')
	CONFTECLA(1,  1,  0,        'G')

	CONFTECLA(0,  5,  0,        'Z')
	CONFTECLA(0,  4,  0,        'X')
	CONFTECLA(0,  3,  0,        'C')
	CONFTECLA(0,  2,  0,        'V')
	CONFTECLA(0,  1,  0,        'B')
	CONFTECLA(0,  0,  0,        0xFF)		// SHIFT

#undef CONFTECLA

#define CONFCONV(tecla, tecla1, onsh1, tecla2, onsh2)		\
				Conversoes[(tecla)].teclasub1 = (tecla1);	\
				Conversoes[(tecla)].onshift1  = (onsh1);	\
				Conversoes[(tecla)].teclasub2 = (tecla2);	\
				Conversoes[(tecla)].onshift2  = (onsh2);
	//       tecla          subst. s/ shift    substituto com shift
	CONFCONV(VK_IGUALMAIS,  'O',      1,       'P', 1)					// = = SHIFT+O	+ = SHIFT+P
	CONFCONV(VK_MENOS,      'I',      1,       0,   0)					// - = SHIFT+I
	CONFCONV(VK_APOSTROFO,  '7',      1,       '2', 1)					// ' = SHIFT+7	" = SHIFT+2
	CONFCONV('2',           0,        0,       'L', 1)					// @ = SHIFT+L
	CONFCONV('6',           0,        0,       'K', 1)					// ^ = SHIFT+K
	CONFCONV('7',           0,        0,       '6', 1)					// & = SHIFT+6
	CONFCONV('8',           0,        0,       '0', 1)					// * = SHIFT+0
	CONFCONV('9',           0,        0,       '8', 1)					// ( = SHIFT+8
	CONFCONV('0',           0,        0,       '9', 1)					// ) = SHIFT+9

#undef CONFCONV

}

//===========================================================================
void KeybAtualiza(DWORD totalcycles) {
	//
};

//===========================================================================
void KeybFinaliza(void) {
	// TODO: finalizar clipboard
}

//===========================================================================
void KeybQueueKeypress (int key, BOOL down, BOOL ascii) {
	if (GetKeyState(VK_MENU) < 0)		/* ALT */
		return;

	int shift = 0;
	if (!ascii) {
		controlStatus = (GetKeyState(VK_CONTROL) < 0);
		if (key == VK_CONTROL)
			return;
		shift = GetKeyState(VK_SHIFT);

		if (down && ((key == VK_CANCEL && GetKeyState(VK_CONTROL) < 0) || key == VK_F12)) {
			// RESET
			MemResetPaging();
			DiskReset();
			ImpressoraReset();
			VideoResetState();
			regs.pc = mem_readw(0xFFFC, 0);
			return;
		} else if (down && key == VK_ESCAPE && g_bClipboardActive) {
			ClipboardDone();
		} else if (down && (key == VK_INSERT) && (GetKeyState(VK_SHIFT) < 0))	{
			// Shift+Insert
			ClipboardInitiatePaste();
			return;
		}
		if (g_bClipboardActive)		/* Não inserir outras teclas enquanto estiver colando */
			return;

		switch(key) {
		case VK_SHIFT:
			key = 0xFF;
			break;

		case VK_UP:
			key = 0x0B;
			break;

		case VK_DOWN:
			key = 0x0A;
			break;

		case VK_RIGHT:
			key = 0x15;
			break;

		case VK_LEFT:
			key = 0x08;
			break;

		case VK_PERGUNTA:
			key = '/';
			break;

		case VK_PONTO:
			key = '.';
			break;

		case VK_VIRGULA:
			key = ',';
			break;

		case VK_DOISPONTOS:
			key = ';';
			break;

		case VK_DECIMAL:
			key = ',';
			break;

		case VK_MULTIPLY:
			key = '8';
			shift = -1;
			break;

		case VK_ADD:
			key = '+';
			break;

		case VK_NPPONTO:
			key = '.';
			break;

		case VK_SUBTRACT:
			key = '-';
			break;

		case VK_DIVIDE:
			key = '/';
			break;

		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
			key -= 0x30;
			break;

		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
			key = 0;
			break;
		}
	}

	key &= 0xFF;

	// Conversões de teclas:

	if (Conversoes[key].teclasub1 && !(shift < 0)) {
		shift = Conversoes[key].onshift1 ? -1 : 0;
		key   = Conversoes[key].teclasub1;
	} else if (Conversoes[key].teclasub2 && (shift < 0)) {
		shift = Conversoes[key].onshift2 ? -1 : 0;
		key   = Conversoes[key].teclasub2;
	}

	if (Teclas[key].linha != 0xFF) {

		if (Teclas[key].flags & FLGINVSH)
			shift = shift < 0 ? 0 : -1;

		if (shift < 0) {
			matriz[0] |= 0x01;
		} else {
			matriz[0] &= ~0x01;
		}

		if (down)
			matriz[Teclas[key].linha] |= 1 << Teclas[key].coluna;
		else
			matriz[Teclas[key].linha] &= ~(1 << Teclas[key].coluna);
	}
}

//===========================================================================
BYTE __stdcall KeybCTRL0 (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	bCTRL = 0;
	return MemReturnRandomData(0);
}

//===========================================================================
BYTE __stdcall KeybCTRL1 (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	bCTRL = 1;
	return MemReturnRandomData(0);
}

//===========================================================================
BYTE __stdcall KeybKBOUT (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (write) {
		bKBOUT = value;
	}
	return bKBOUT;
}

//===========================================================================
BYTE __stdcall KeybKBIN (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	int  l;
	static int romv = 0;		/* Contabiliza quantos scans completos foram feitos pela ROM */
	//BYTE result = TapeCASIN(programcounter, address, write, value);
	BYTE result = 0x80;

	if (bCTRL && controlStatus && !g_bClipboardActive)
		return result | 0x01;

	for (l = 0; l < 8; l++) {
		if (bKBOUT & (1 << l)) {
			result |= JoyVerificaTecla(l);
			result |= matriz[l];
		}
	}

	if (ultCicloP) {
		if ((g_nCumulativeCycles - ultCicloP) > 9777) {						/* Mantém pressionada por 10 ms */
			if(*lptstr == 0) {
				ClipboardDone();
			} else {
				KeybQueueKeypress(ClipboardCurrChar(true), false, true);	/* Tecla liberada */
			}
			ultCicloP = 0;
			ultCicloL = g_nCumulativeCycles;
		}
	} else if (bKBOUT == 0x01 && bKBOUT_ant == 0x80)
		bKBOUT_ant = 0x01;
	else if (bKBOUT == 0x02 && bKBOUT_ant == 0x01)
		bKBOUT_ant = 0x02;
	else if (bKBOUT == 0x04 && bKBOUT_ant == 0x02)
		bKBOUT_ant = 0x04;
	else if (bKBOUT == 0x08 && bKBOUT_ant == 0x04)
		bKBOUT_ant = 0x08;
	else if (bKBOUT == 0x10 && bKBOUT_ant == 0x08)
		bKBOUT_ant = 0x10;
	else if (bKBOUT == 0x20 && bKBOUT_ant == 0x10)
		bKBOUT_ant = 0x20;
	else if (bKBOUT == 0x40 && bKBOUT_ant == 0x20)
		bKBOUT_ant = 0x40;
	else if (bKBOUT == 0x80 && bKBOUT_ant == 0x40) {
		bKBOUT_ant = 0x80;

		if(g_bPasteFromClipboard) {
			ClipboardInit();
			romv = 0;
		}

		if(g_bClipboardActive) {
			if (g_nCumulativeCycles - ultCicloL > 9777*3) {			/* Espera 30 ms com a tecla liberada */
				if(*lptstr == 0) {
					ClipboardDone();
					ultCicloP = 0;
				} else if (romv == 2) {								/* Somente após 3 leituras completas da ROM */
					KeybQueueKeypress(ClipboardCurrChar(false), true, true);	/* Tecla pressionada */
					ultCicloP = g_nCumulativeCycles;
				}
				ultCicloL = g_nCumulativeCycles;
			}
		}
		if (++romv == 3)
			romv = 0;
	}
	return result;
}

// EOF