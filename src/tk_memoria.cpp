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

// Gerencia a memória

#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_memoria.h"
#include "tk_cpu.h"
#include "tk_joystick.h"
#include "tk_video.h"
#include "tk_som.h"
#include "tk_teclado.h"
#include "tk_debug.h"
#include "tk_registro.h"
#include "tk_tape.h"
#include "tk_janela.h"
#include "tk_impressora.h"
#include "tk_disco.h"

#define  MF_RAMROM16   0x00000001
//#define  MF_BANCO2     0x00000002
//#define  MF_ESCREVERAM 0x00000004

#define SW_RAMROM16    (memmode & MF_RAMROM16)
//#define SW_BANCO2       (memmode & MF_BANCO2)
//#define SW_ESCREVERAM   (memmode & MF_ESCREVERAM)

iofunction ioread[0x100]  = {KeybKBOUT,		// $C000
                             KeybKBOUT,		// $C001
                             KeybKBOUT,		// $C002
                             KeybKBOUT,		// $C003
                             KeybKBOUT,		// $C004
                             KeybKBOUT,		// $C005
                             KeybKBOUT,		// $C006
                             KeybKBOUT,		// $C007
                             KeybKBOUT,		// $C008
                             KeybKBOUT,		// $C009
                             KeybKBOUT,		// $C00A
                             KeybKBOUT,		// $C00B
                             KeybKBOUT,		// $C00C
                             KeybKBOUT,		// $C00D
                             KeybKBOUT,		// $C00E
                             KeybKBOUT,		// $C00F
                             KeybKBIN,		// $C010
                             KeybKBIN,		// $C011
                             KeybKBIN,		// $C012
                             KeybKBIN,		// $C013
                             KeybKBIN,		// $C014
                             KeybKBIN,		// $C015
                             KeybKBIN,		// $C016
                             KeybKBIN,		// $C017
                             KeybKBIN,		// $C018
                             KeybKBIN,		// $C019
                             KeybKBIN,		// $C01A
                             KeybKBIN,		// $C01B
                             KeybKBIN,		// $C01C
                             KeybKBIN,		// $C01D
                             KeybKBIN,		// $C01E
                             KeybKBIN,		// $C01F
                             TapeCASOUT,	// $C020
                             TapeCASOUT,	// $C021
                             TapeCASOUT,	// $C022
                             TapeCASOUT,	// $C023
                             TapeCASOUT,	// $C024
                             TapeCASOUT,	// $C025
                             TapeCASOUT,	// $C026
                             TapeCASOUT,	// $C027
                             TapeCASOUT,	// $C028
                             TapeCASOUT,	// $C029
                             TapeCASOUT,	// $C02A
                             TapeCASOUT,	// $C02B
                             TapeCASOUT,	// $C02C
                             TapeCASOUT,	// $C02D
                             TapeCASOUT,	// $C02E
                             TapeCASOUT,	// $C02F
                             SpkrToggle,	// $C030
                             SpkrToggle,	// $C031
                             SpkrToggle,	// $C032
                             SpkrToggle,	// $C033
                             SpkrToggle,	// $C034
                             SpkrToggle,	// $C035
                             SpkrToggle,	// $C036
                             SpkrToggle,	// $C037
                             SpkrToggle,	// $C038
                             SpkrToggle,	// $C039
                             SpkrToggle,	// $C03A
                             SpkrToggle,	// $C03B
                             SpkrToggle,	// $C03C
                             SpkrToggle,	// $C03D
                             SpkrToggle,	// $C03E
                             SpkrToggle,	// $C03F
                             NullIo,		// $C040
                             NullIo,		// $C041
                             NullIo,		// $C042
                             NullIo,		// $C043
                             NullIo,		// $C044
                             NullIo,		// $C045
                             NullIo,		// $C046
                             NullIo,		// $C047
                             NullIo,		// $C048
                             NullIo,		// $C049
                             NullIo,		// $C04A
                             NullIo,		// $C04B
                             NullIo,		// $C04C
                             NullIo,		// $C04D
                             NullIo,		// $C04E
                             NullIo,		// $C04F
                             VideoSetMode,	// $C050
                             VideoSetMode,	// $C051
                             TapeMOTOR,		// $C052
                             TapeMOTOR,		// $C053
                             VideoSetMode,	// $C054
                             VideoSetMode,	// $C055
                             TapeMOTOR,		// $C056
                             TapeMOTOR,		// $C057
                             ImpressoraStrobe,// $C058
                             ImpressoraStrobe,// $C059
                             MemSetPaging,	// $C05A
                             MemSetPaging,	// $C05B
                             NullIo,		// $C05C
                             NullIo,		// $C05D
                             KeybCTRL0,		// $C05E
                             KeybCTRL1,		// $C05F
                             NullIo,		// $C060
                             NullIo,		// $C061
                             NullIo,		// $C062
                             NullIo,		// $C063
                             NullIo,		// $C064
                             NullIo,		// $C065
                             NullIo,		// $C066
                             NullIo,		// $C067
                             NullIo,		// $C068
                             NullIo,		// $C069
                             NullIo,		// $C06A
                             NullIo,		// $C06B
                             NullIo,		// $C06C
                             NullIo,		// $C06D
                             NullIo,		// $C06E
                             NullIo,		// $C06F
                             TapeCASIN,		// $C070
                             TapeCASIN2,	// $C071
                             NullIo,		// $C072
                             NullIo,		// $C073
                             NullIo,		// $C074
                             NullIo,		// $C075
                             NullIo,		// $C076
                             NullIo,		// $C077
                             NullIo,		// $C078
                             NullIo,		// $C079
                             NullIo,		// $C07A
                             NullIo,		// $C07B
                             NullIo,		// $C07C
                             NullIo,		// $C07D
                             NullIo,		// $C07E
                             NullIo,		// $C07F
                             MemSetPaging,	// $C080
                             MemSetPaging,	// $C081
                             MemSetPaging,	// $C082
                             MemSetPaging,	// $C083
                             MemSetPaging,	// $C084
                             MemSetPaging,	// $C085
                             MemSetPaging,	// $C086
                             MemSetPaging,	// $C087
                             MemSetPaging,	// $C088
                             MemSetPaging,	// $C089
                             MemSetPaging,	// $C08A
                             MemSetPaging,	// $C08B
                             NullIo,		// $C08C
                             NullIo,		// $C08D
                             NullIo,		// $C08E
                             NullIo,		// $C08F
                             NullIo,		// $C090
                             NullIo,		// $C091
                             NullIo,		// $C092
                             NullIo,		// $C093
                             NullIo,		// $C094
                             NullIo,		// $C095
                             NullIo,		// $C096
                             NullIo,		// $C097
                             NullIo,		// $C098
                             NullIo,		// $C099
                             NullIo,		// $C09A
                             NullIo,		// $C09B
                             NullIo,		// $C09C
                             NullIo,		// $C09D
                             NullIo,		// $C09E
                             NullIo,		// $C09F
                             NullIo,		// $C0A0
                             NullIo,		// $C0A1
                             NullIo,		// $C0A2
                             NullIo,		// $C0A3
                             NullIo,		// $C0A4
                             NullIo,		// $C0A5
                             NullIo,		// $C0A6
                             NullIo,		// $C0A7
                             NullIo,		// $C0A8
                             NullIo,		// $C0A9
                             NullIo,		// $C0AA
                             NullIo,		// $C0AB
                             NullIo,		// $C0AC
                             NullIo,		// $C0AD
                             NullIo,		// $C0AE
                             NullIo,		// $C0AF
                             NullIo,		// $C0B0
                             NullIo,		// $C0B1
                             NullIo,		// $C0B2
                             NullIo,		// $C0B3
                             NullIo,		// $C0B4
                             NullIo,		// $C0B5
                             NullIo,		// $C0B6
                             NullIo,		// $C0B7
                             NullIo,		// $C0B8
                             NullIo,		// $C0B9
                             NullIo,		// $C0BA
                             NullIo,		// $C0BB
                             NullIo,		// $C0BC
                             NullIo,		// $C0BD
                             NullIo,		// $C0BE
                             NullIo,		// $C0BF
                             NullIo,		// $C0C0
                             NullIo,		// $C0C1
                             NullIo,		// $C0C2
                             NullIo,		// $C0C3
                             NullIo,		// $C0C4
                             NullIo,		// $C0C5
                             NullIo,		// $C0C6
                             NullIo,		// $C0C7
                             NullIo,		// $C0C8
                             NullIo,		// $C0C9
                             NullIo,		// $C0CA
                             NullIo,		// $C0CB
                             NullIo,		// $C0CC
                             NullIo,		// $C0CD
                             NullIo,		// $C0CE
                             NullIo,		// $C0CF
                             NullIo,		// $C0D0
                             NullIo,		// $C0D1
                             NullIo,		// $C0D2
                             NullIo,		// $C0D3
                             NullIo,		// $C0D4
                             NullIo,		// $C0D5
                             NullIo,		// $C0D6
                             NullIo,		// $C0D7
                             NullIo,		// $C0D8
                             NullIo,		// $C0D9
                             NullIo,		// $C0DA
                             NullIo,		// $C0DB
                             NullIo,		// $C0DC
                             NullIo,		// $C0DD
                             NullIo,		// $C0DE
                             NullIo,		// $C0DF
                             NullIo,		// $C0E0
                             NullIo,		// $C0E1
                             NullIo,		// $C0E2
                             NullIo,		// $C0E3
                             NullIo,		// $C0E4
                             NullIo,		// $C0E5
                             NullIo,		// $C0E6
                             NullIo,		// $C0E7
                             NullIo,		// $C0E8
                             NullIo,		// $C0E9
                             NullIo,		// $C0EA
                             NullIo,		// $C0EB
                             NullIo,		// $C0EC
                             NullIo,		// $C0ED
                             NullIo,		// $C0EE
                             NullIo,		// $C0EF
                             NullIo,		// $C0F0
                             NullIo,		// $C0F1
                             NullIo,		// $C0F2
                             NullIo,		// $C0F3
                             NullIo,		// $C0F4
                             NullIo,		// $C0F5
                             NullIo,		// $C0F6
                             NullIo,		// $C0F7
                             NullIo,		// $C0F8
                             NullIo,		// $C0F9
                             NullIo,		// $C0FA
                             NullIo,		// $C0FB
                             NullIo,		// $C0FC
                             NullIo,		// $C0FD
                             NullIo,		// $C0FE
                             NullIo			// $C0FF
};

iofunction iowrite[0x100] = {KeybKBOUT,		// $C000
                             KeybKBOUT,		// $C001
                             KeybKBOUT,		// $C002
                             KeybKBOUT,		// $C003
                             KeybKBOUT,		// $C004
                             KeybKBOUT,		// $C005
                             KeybKBOUT,		// $C006
                             KeybKBOUT,		// $C007
                             KeybKBOUT,		// $C008
                             KeybKBOUT,		// $C009
                             KeybKBOUT,		// $C00A
                             KeybKBOUT,		// $C00B
                             KeybKBOUT,		// $C00C
                             KeybKBOUT,		// $C00D
                             KeybKBOUT,		// $C00E
                             KeybKBOUT,		// $C00F
                             NullIo,		// $C010
                             NullIo,		// $C011
                             NullIo,		// $C012
                             NullIo,		// $C013
                             NullIo,		// $C014
                             NullIo,		// $C015
                             NullIo,		// $C016
                             NullIo,		// $C017
                             NullIo,		// $C018
                             NullIo,		// $C019
                             NullIo,		// $C01A
                             NullIo,		// $C01B
                             NullIo,		// $C01C
                             NullIo,		// $C01D
                             NullIo,		// $C01E
                             NullIo,		// $C01F
                             TapeCASOUT,	// $C020
                             TapeCASOUT,	// $C021
                             TapeCASOUT,	// $C022
                             TapeCASOUT,	// $C023
                             TapeCASOUT,	// $C024
                             TapeCASOUT,	// $C025
                             TapeCASOUT,	// $C026
                             TapeCASOUT,	// $C027
                             TapeCASOUT,	// $C028
                             TapeCASOUT,	// $C029
                             TapeCASOUT,	// $C02A
                             TapeCASOUT,	// $C02B
                             TapeCASOUT,	// $C02C
                             TapeCASOUT,	// $C02D
                             TapeCASOUT,	// $C02E
                             TapeCASOUT,	// $C02F
                             SpkrToggle,	// $C030
                             SpkrToggle,	// $C031
                             SpkrToggle,	// $C032
                             SpkrToggle,	// $C033
                             SpkrToggle,	// $C034
                             SpkrToggle,	// $C035
                             SpkrToggle,	// $C036
                             SpkrToggle,	// $C037
                             SpkrToggle,	// $C038
                             SpkrToggle,	// $C039
                             SpkrToggle,	// $C03A
                             SpkrToggle,	// $C03B
                             SpkrToggle,	// $C03C
                             SpkrToggle,	// $C03D
                             SpkrToggle,	// $C03E
                             SpkrToggle,	// $C03F
                             NullIo,		// $C040
                             NullIo,		// $C041
                             NullIo,		// $C042
                             NullIo,		// $C043
                             NullIo,		// $C044
                             NullIo,		// $C045
                             NullIo,		// $C046
                             NullIo,		// $C047
                             NullIo,		// $C048
                             NullIo,		// $C049
                             NullIo,		// $C04A
                             NullIo,		// $C04B
                             NullIo,		// $C04C
                             NullIo,		// $C04D
                             NullIo,		// $C04E
                             NullIo,		// $C04F
                             VideoSetMode,	// $C050
                             VideoSetMode,	// $C051
                             TapeMOTOR,		// $C052
                             TapeMOTOR,		// $C053
                             VideoSetMode,	// $C054
                             VideoSetMode,	// $C055
                             TapeMOTOR,		// $C056
                             TapeMOTOR,		// $C057
                             ImpressoraStrobe,// $C058
                             ImpressoraStrobe,// $C059
                             MemSetPaging,	// $C05A
                             MemSetPaging,	// $C05B
                             NullIo,		// $C05C
                             NullIo,		// $C05D
                             KeybCTRL0,		// $C05E
                             KeybCTRL1,		// $C05F
                             NullIo,		// $C060
                             NullIo,		// $C061
                             NullIo,		// $C062
                             NullIo,		// $C063
                             NullIo,		// $C064
                             NullIo,		// $C065
                             NullIo,		// $C066
                             NullIo,		// $C067
                             NullIo,		// $C068
                             NullIo,		// $C069
                             NullIo,		// $C06A
                             NullIo,		// $C06B
                             NullIo,		// $C06C
                             NullIo,		// $C06D
                             NullIo,		// $C06E
                             NullIo,		// $C06F
                             NullIo,		// $C070
                             NullIo,		// $C071
                             NullIo,		// $C072
                             NullIo,		// $C073
                             NullIo,		// $C074
                             NullIo,		// $C075
                             NullIo,		// $C076
                             NullIo,		// $C077
                             NullIo,		// $C078
                             NullIo,		// $C079
                             NullIo,		// $C07A
                             NullIo,		// $C07B
                             NullIo,		// $C07C
                             NullIo,		// $C07D
                             NullIo,		// $C07E
                             NullIo,		// $C07F
                             MemSetPaging,	// $C080
                             MemSetPaging,	// $C081
                             MemSetPaging,	// $C082
                             MemSetPaging,	// $C083
                             MemSetPaging,	// $C084
                             MemSetPaging,	// $C085
                             MemSetPaging,	// $C086
                             MemSetPaging,	// $C087
                             MemSetPaging,	// $C088
                             MemSetPaging,	// $C089
                             MemSetPaging,	// $C08A
                             MemSetPaging,	// $C08B
                             NullIo,		// $C08C
                             NullIo,		// $C08D
                             NullIo,		// $C08E
                             NullIo,		// $C08F
                             NullIo,		// $C090
                             NullIo,		// $C091
                             NullIo,		// $C092
                             NullIo,		// $C093
                             NullIo,		// $C094
                             NullIo,		// $C095
                             NullIo,		// $C096
                             NullIo,		// $C097
                             NullIo,		// $C098
                             NullIo,		// $C099
                             NullIo,		// $C09A
                             NullIo,		// $C09B
                             NullIo,		// $C09C
                             NullIo,		// $C09D
                             NullIo,		// $C09E
                             NullIo,		// $C09F
                             NullIo,		// $C0A0
                             NullIo,		// $C0A1
                             NullIo,		// $C0A2
                             NullIo,		// $C0A3
                             NullIo,		// $C0A4
                             NullIo,		// $C0A5
                             NullIo,		// $C0A6
                             NullIo,		// $C0A7
                             NullIo,		// $C0A8
                             NullIo,		// $C0A9
                             NullIo,		// $C0AA
                             NullIo,		// $C0AB
                             NullIo,		// $C0AC
                             NullIo,		// $C0AD
                             NullIo,		// $C0AE
                             NullIo,		// $C0AF
                             NullIo,		// $C0B0
                             NullIo,		// $C0B1
                             NullIo,		// $C0B2
                             NullIo,		// $C0B3
                             NullIo,		// $C0B4
                             NullIo,		// $C0B5
                             NullIo,		// $C0B6
                             NullIo,		// $C0B7
                             NullIo,		// $C0B8
                             NullIo,		// $C0B9
                             NullIo,		// $C0BA
                             NullIo,		// $C0BB
                             NullIo,		// $C0BC
                             NullIo,		// $C0BD
                             NullIo,		// $C0BE
                             NullIo,		// $C0BF
                             NullIo,		// $C0C0
                             NullIo,		// $C0C1
                             NullIo,		// $C0C2
                             NullIo,		// $C0C3
                             NullIo,		// $C0C4
                             NullIo,		// $C0C5
                             NullIo,		// $C0C6
                             NullIo,		// $C0C7
                             NullIo,		// $C0C8
                             NullIo,		// $C0C9
                             NullIo,		// $C0CA
                             NullIo,		// $C0CB
                             NullIo,		// $C0CC
                             NullIo,		// $C0CD
                             NullIo,		// $C0CE
                             NullIo,		// $C0CF
                             NullIo,		// $C0D0
                             NullIo,		// $C0D1
                             NullIo,		// $C0D2
                             NullIo,		// $C0D3
                             NullIo,		// $C0D4
                             NullIo,		// $C0D5
                             NullIo,		// $C0D6
                             NullIo,		// $C0D7
                             NullIo,		// $C0D8
                             NullIo,		// $C0D9
                             NullIo,		// $C0DA
                             NullIo,		// $C0DB
                             NullIo,		// $C0DC
                             NullIo,		// $C0DD
                             NullIo,		// $C0DE
                             NullIo,		// $C0DF
                             NullIo,		// $C0E0
                             NullIo,		// $C0E1
                             NullIo,		// $C0E2
                             NullIo,		// $C0E3
                             NullIo,		// $C0E4
                             NullIo,		// $C0E5
                             NullIo,		// $C0E6
                             NullIo,		// $C0E7
                             NullIo,		// $C0E8
                             NullIo,		// $C0E9
                             NullIo,		// $C0EA
                             NullIo,		// $C0EB
                             NullIo,		// $C0EC
                             NullIo,		// $C0ED
                             NullIo,		// $C0EE
                             NullIo,		// $C0EF
                             NullIo,		// $C0F0
                             NullIo,		// $C0F1
                             NullIo,		// $C0F2
                             NullIo,		// $C0F3
                             NullIo,		// $C0F4
                             NullIo,		// $C0F5
                             NullIo,		// $C0F6
                             NullIo,		// $C0F7
                             NullIo,		// $C0F8
                             NullIo,		// $C0F9
                             NullIo,		// $C0FA
                             NullIo,		// $C0FB
                             NullIo,		// $C0FC
                             NullIo,		// $C0FD
                             NullIo,		// $C0FE
                             NullIo			// $C0FF
};

BYTE*   memread[0x100];
BYTE*   memwrite[0x100];
BYTE*   memdirty     = NULL;
BYTE*   memmain      = NULL;
BYTE*   memrom       = NULL;
BYTE*	memslot      = NULL;
//BYTE*   memaux       = NULL;
DWORD   memmode      = MF_RAMROM16;
BYTE	SlotAux      = 0;
BYTE    NewSlotAux   = 0;

void	UpdatePaging (BOOL initialize, BOOL updatewriteonly);

//===========================================================================
BYTE __stdcall NullIo (WORD programcounter, BYTE address, BYTE write, BYTE value)
{
	return 0xFF;
}

//===========================================================================
void ResetPaging (BOOL initialize)
{
//	lastwriteram = 0;
	memmode      = MF_RAMROM16;
	UpdatePaging(initialize, 0);
}

// Atualiza Memória
//===========================================================================
void UpdatePaging (BOOL initialize, BOOL updatewriteonly)
{
	int loop;

	// UPDATE THE PAGING TABLES BASED ON THE NEW PAGING SWITCH VALUES
	for (loop = 0; loop <= 0xBF; loop++)		// 0x0000 à 0xBFFF
	{
		memread[loop]  = memmain+(loop << 8);
		memwrite[loop] = memmain+(loop << 8);
	}
	for (loop = 0xC1; loop <= 0xC2; loop++)		// 0xC100 à 0xC200
	{
		memread[loop]  = SlotAux    ? memslot+(loop << 8)-0xC100
									: SW_RAMROM16 ? memrom+(loop << 8)-0xC000
												: memmain+(loop << 8);
		memwrite[loop] = SlotAux    ? NULL
									: SW_RAMROM16 ? NULL
												: memmain+(loop << 8);
	}

	for (loop = 0xC2; loop <= 0xFF; loop++)		// 0xC200 à 0xFFFF
	{
		memread[loop]  = SW_RAMROM16 ? memrom+(loop << 8)-0xC000
									: memmain+(loop << 8);
		memwrite[loop] = SW_RAMROM16 ? NULL
									: memmain+(loop << 8);
	}
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
int MemAtualizaSlotAux()
{
	// Retira Slot Auxiliar
	switch (SlotAux)
	{
		case 0: // Vazio
			//
		break;

		case 1: // Interface de Disco
			DiscoRetiraSlotAux();
		break;
	} // switch

	SlotAux = NewSlotAux;

	// Insere Slot Auxiliar
	switch (SlotAux)
	{
		case 0: // Vazio
			return MemRetiraSlotAux();
		break;

		case 1: // Interface de Disco
			if (!DiscoConfiguraSlotAux())
			{
				SlotAux = 0;
				return 0;  // Retorna Erro
			}
		break;
	} // switch
	
	return 1;  // Sem Erros
}

//===========================================================================
int MemRetiraSlotAux()
{
	int i;

	for (i=0; i < 16; i++)
	{
		ioread[0x90+i] = NullIo;
		iowrite[0x90+i] = NullIo;
	}
	UpdatePaging(0, 0);
	return 1;
}

//===========================================================================
int MemInsereSlotAux(  iofunction ioarrayread[0x10],
						iofunction ioarraywrite[0x10],
						char *NomeFirmware)
{
	char Caminho[MAX_PATH];
	char temp1[MAX_PATH];
	int  i;
	FILE *file;

	if (stricmp(NomeFirmware,"NADA"))
	{
		sprintf(Caminho,"%s%s_%s%s",progdir, "Firmware", NomeFirmware, ".rom");
		file = fopen(Caminho, "rb");
		if (file == NULL)
		{
			sprintf(temp1, "O firmware %s não foi achado.", NomeFirmware);
			FrameMostraMensagemErro(temp1);
			return 0;	// Retorna erro
		}
		fread(memslot, 0x100, 1, file);
		fclose(file);
	}
	else
	{
		memset(memslot, 0xFF, 256);
	}
	for (i=0; i < 16; i++)
	{
		ioread[0x90+i] = ioarrayread[i];
		iowrite[0x90+i] = ioarraywrite[i];
	}

	UpdatePaging(0,0);
	return 1;  // Sem Erros
}


//===========================================================================
BOOL MemImportar(WORD EndInicial)
{
#ifdef _WINDOWS
	OPENFILENAME ofn;
#endif // _WINDOWS
	FILE  *Arquivo;
	char  *Buffer;
	DWORD TamMem;
	char  directory[MAX_PATH] = "";
	char  filename[MAX_PATH]  = "";
	unsigned int c;

#ifdef _WINDOWS
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize     = sizeof(OPENFILENAME);
	ofn.hwndOwner       = (HWND)framewindow;
	ofn.hInstance       = (HINSTANCE)instance;
	ofn.lpstrFilter     = "Todos os Arquivos\0*.*\0";
	ofn.lpstrFile       = filename;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = directory;
	ofn.Flags           = OFN_PATHMUSTEXIST;
	ofn.lpstrTitle      = "Escolha o Arquivo";
	if (GetOpenFileName(&ofn))
#else
	if (0)
#endif
	{
		Arquivo = fopen(filename, "rb");
		if (!Arquivo)
		{
			FrameMostraMensagemErro("Erro ao abrir o arquivo!");
			return -1;
		}
		fseek(Arquivo, 0, SEEK_END);
		TamMem = ftell(Arquivo);
		if ((EndInicial+TamMem) > 0xC000)
		{
			FrameMostraMensagemErro("O conteúdo do arquivo excede a capacidade de memória do TK2000");
			return -1;
		}
		Buffer = (char *)malloc(TamMem);
		fseek(Arquivo, 0, SEEK_SET);
		fread(Buffer, TamMem, 1, Arquivo);
		//memcpy((void *)(mem+EndInicial), (const void*)Buffer, (size_t)(TamMem));
		for (c = 0; c < TamMem; c++)
			mem_writeb(EndInicial + c, Buffer[c]);
		fclose(Arquivo);
		return 0;
	}
	return -1;
}

//===========================================================================
BOOL MemExportar(WORD EndInicial, WORD EndFinal)
{
#ifdef _WINDOWS
	OPENFILENAME ofn;
#endif // _WINDOWS
	FILE  *Arquivo;
	char  *Buffer;
	DWORD TamMem;
	char  directory[MAX_PATH] = "";
	char  filename[MAX_PATH]  = "";
	unsigned int c;

	TamMem = EndFinal - EndInicial + 1;
	Buffer = (char *)malloc(TamMem);
	for (c = 0; c < TamMem; c++)
		Buffer[c] = mem_readb(EndInicial + c, 0);
	//memcpy((void *)Buffer, (const void*)(mem+EndInicial), (size_t)(TamMem));
#ifdef _WINDOWS
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize     = sizeof(OPENFILENAME);
	ofn.hwndOwner       = (HWND)framewindow;
	ofn.hInstance       = (HINSTANCE)instance;
	ofn.lpstrFilter     = "Todos os Arquivos\0*.*\0";
	ofn.lpstrFile       = filename;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = directory;
	ofn.Flags           = OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle      = "Escolha o Arquivo";
	if (GetSaveFileName(&ofn))
#else // _WINDOWS
	if (0)
#endif // _WINDOWS
	{
		Arquivo = fopen(filename, "wb");
		if (!Arquivo)
		{
			FrameMostraMensagemErro("Erro ao criar arquivo!");
			return -1;
		}
		if (!fwrite(Buffer, TamMem, 1, Arquivo))
		{
			FrameMostraMensagemErro("Erro ao criar arquivo!");
			fclose(Arquivo);
			return -1;
		}
		fclose(Arquivo);
		return 0;
	}
	return -1;
}

//===========================================================================
BYTE mem_readb(WORD endereco, BOOL AcessarIO)
{
	BYTE* page;
	BYTE  result;

	if ((endereco & 0xFF00) == 0xC000) 
	{
		if (AcessarIO)
			result = ioread[endereco & 0xFF](regs.pc,(BYTE)endereco, false, 0);
		else
			result = 0xFF;
	}
	else
	{
		page = memread[endereco >> 8];	
		result = *(page+(endereco & 0xFF));
	}
	return result;
}

//===========================================================================
WORD mem_readw(WORD endereco, BOOL AcessarIO)
{
	BYTE* page;
	WORD  result;

	if ((endereco & 0xFF00) == 0xC000)
	{
		if (AcessarIO)
			result = (ioread[endereco & 0xFF](regs.pc,(BYTE)endereco, false, 0)) |
					 (ioread[++endereco & 0xFF](regs.pc,(BYTE)endereco, false, 0) << 8);
		else
			result = 0xFFFF;
	}
	else
	{
		page = memread[endereco >> 8];
		result = *(BYTE*)(page+(endereco & 0xFF));
		page = memread[++endereco >> 8];
		result |= (*(BYTE*)(page+(endereco & 0xFF))) << 8;
	}
	return result;
}

//===========================================================================
void mem_writeb(WORD endereco, BYTE valor)
{
	BYTE* page;
	
	memdirty[endereco >> 8] = 0xFF;

	if ((endereco & 0xFF00) == 0xC000)
	{
		iowrite[endereco & 0xFF](regs.pc,(BYTE)endereco, true, (BYTE)(valor));
	}
	else
	{
		page = memwrite[endereco >> 8];	
		if (page)
			*(page+(endereco & 0xFF)) = (BYTE)(valor);
	}
}

//===========================================================================
void mem_writew(WORD endereco, WORD valor)
{
	BYTE* page;
	
	memdirty[endereco >> 8] = 0xFF;

	if ((endereco & 0xFF00) == 0xC000)
	{
		iowrite[endereco & 0xFF](regs.pc,(BYTE)endereco, true, (BYTE)(valor));
		iowrite[++endereco & 0xFF](regs.pc,(BYTE)endereco, true, (BYTE)(valor >> 8));
	}
	else
	{
		page = memwrite[endereco >> 8];	
		if (page)
			*(BYTE*)(page+(endereco & 0xFF)) = valor & 0xFF;
		page = memwrite[++endereco >> 8];	
		if (page)
			*(BYTE*)(page+(endereco & 0xFF)) = valor >> 8;
	}
}

//===========================================================================
void MemDestroy()
{
//	free(memaux);
	free(memdirty);
	free(memmain);
	free(memrom);
	free(memslot);
//	memaux     = NULL;
	memdirty   = NULL;
	memmain    = NULL;
	memrom     = NULL;
	memslot    = NULL;
	memset(memwrite, 0, sizeof(memwrite));
	memset(memread,  0, sizeof(memread ));
}

//===========================================================================
BYTE* MemGetMainPtr (WORD offset)
{
	return memmain+offset;
}

//===========================================================================
void MemInitialize ()
{

	// ALLOCATE MEMORY FOR THE APPLE MEMORY IMAGE AND ASSOCIATED DATA
	// STRUCTURES
	//
	char  filename[MAX_PATH];
	FILE  *file;

//	memaux     = (BYTE*)malloc(0x010000); //VirtualAlloc(NULL,0x10000,MEM_COMMIT,PAGE_READWRITE);
	memdirty   = (BYTE*)malloc(0x000100); //VirtualAlloc(NULL,0x0100 ,MEM_COMMIT,PAGE_READWRITE);
	memmain    = (BYTE*)malloc(0x010000); //VirtualAlloc(NULL,0x10000,MEM_COMMIT,PAGE_READWRITE);
	memrom     = (BYTE*)malloc(0x005000); //VirtualAlloc(NULL,0x5000 ,MEM_COMMIT,PAGE_READWRITE);
	memslot    = (BYTE*)malloc(0x000100); //VirtualAlloc(NULL,0x5000 ,MEM_COMMIT,PAGE_READWRITE);
	if (/*(!memaux) || */(!memdirty) ||	(!memmain) || (!memrom) || (!memslot))
	{
		FrameMostraMensagemErro("Não foi possível alocar memória para o emulador.\n"
				"A execução não será possível.");
		exit(1);
	}

	memset(memslot, 0xFF, 0x100);
	// READ THE APPLE FIRMWARE ROMS INTO THE ROM IMAGE
	memset(memrom, 0xFF, 0x5000);
	strcpy(filename, progdir);
	strcat(filename, NOMEARQROM);
	file = fopen(filename, "rb");
	if (file == NULL)
	{
		FrameMostraMensagemErro("A imagem de ROM não foi achada: "NOMEARQROM".");
		exit(1);
	}
	fread(memrom, 0x4000, 1, file);
	fclose(file);

	MemReset();
}

//===========================================================================
void MemReset ()
{
	// INITIALIZE THE PAGING TABLES
	memset(memread,  0, 256*sizeof(BYTE*));
	memset(memwrite, 0, 256*sizeof(BYTE*));

	// INITIALIZE THE RAM IMAGES
//	memset(memaux , 0x00, 0x10000);
	memset(memmain, 0x00, 0x10000);
	memset(memmain+0x2000, 0xFF, 0x1000); // Simula sujeira na tela
	memset(memmain+0xA000, 0xFF, 0x1000); // Simula sujeira na tela

	// INITIALIZE PAGING, FILLING IN THE 64K MEMORY IMAGE
	ResetPaging(1);

	// INITIALIZE THE CPU
	CpuInitialize();
}

//===========================================================================
void MemResetPaging()
{
	ResetPaging(0);
}

//===========================================================================
BYTE MemReturnRandomData (BYTE highbit)
{
	return 0x7F | (highbit ? 0x80 : 0x00);
}

// SoftSwitches
//===========================================================================
BYTE __stdcall MemSetPaging (WORD programcounter, BYTE address, BYTE write, BYTE value)
{
	DWORD lastmemmode = memmode;

	switch (address)
	{
		case 0x5A: memmode |=  MF_RAMROM16;    break;
		case 0x5B: memmode &= ~MF_RAMROM16;    break;
	}

	if (memmode != lastmemmode)
		UpdatePaging(0, 0);

	return MemReturnRandomData(0);
}

// EOF