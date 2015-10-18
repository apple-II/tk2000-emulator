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

// Ofecere suporte a depuração de programas

#include "tk_stdhdr.h"
#include "tk_debug.h"
#include "tk_cpu.h"
#include "tk_main.h"
#include "tk_janela.h"
#include "tk_video.h"
#include "tk_teclado.h"
#include "tk_memoria.h"

#define  BREAKPOINTS     5
#define  COMMANDLINES    5
#define  COMMANDS        63
#define  MAXARGS         40
#define  SOURCELINES     19
#define  STACKLINES      9
#define  WATCHES         5

#define  SCREENSPLIT1    356
#define  SCREENSPLIT2    456

#define  INVALID1        1
#define  INVALID2        2
#define  INVALID3        3
#define  ADDR_IMM        4
#define  ADDR_ABS        5
#define  ADDR_ZPG        6
#define  ADDR_ABSX       7
#define  ADDR_ABSY       8
#define  ADDR_ZPGX       9
#define  ADDR_ZPGY       10
#define  ADDR_REL        11
#define  ADDR_INDX       12
#define  ADDR_INDY       13
#define  ADDR_IABS       14

#define  COLOR_INSTBKG   0
#define  COLOR_INSTTEXT  1
#define  COLOR_INSTBP    2
#define  COLOR_DATABKG   3
#define  COLOR_DATATEXT  4
#define  COLOR_STATIC    5
#define  COLOR_BPDATA    6
#define  COLOR_COMMAND   7
#define  _COLORS         8

typedef BOOL (*cmdfunction)(int);

BOOL CmdBlackWhite (int args);
BOOL CmdBreakpointAdd (int args);
BOOL CmdBreakpointClear (int args);
BOOL CmdBreakpointDisable (int args);
BOOL CmdBreakpointEnable (int args);
BOOL CmdCycleClear (int args);
BOOL CmdCodeDump (int args);
BOOL CmdColor (int args);
BOOL CmdExtBenchmark (int args);
BOOL CmdFeedKey (int args);
BOOL CmdFlagSet (int args);
BOOL CmdGo (int args);
BOOL CmdInput (int args);
BOOL CmdInternalCodeDump (int args);
BOOL CmdInternalMemoryDump (int args);
BOOL CmdLineDown (int args);
BOOL CmdLineUp (int args);
BOOL CmdMemoryDump (int args);
BOOL CmdMemoryEnter (int args);
BOOL CmdMemoryFill (int args);
BOOL CmdOutput (int args);
BOOL CmdPageDown (int args);
BOOL CmdPageUp (int args);
BOOL CmdProfile (int args);
BOOL CmdRegisterSet (int args);
BOOL CmdTrace (int args);
BOOL CmdTraceOut (int args);
BOOL CmdTraceFile (int args);
BOOL CmdTraceLine (int args);
BOOL CmdViewOutput (int args);
BOOL CmdWatchAdd (int args);
BOOL CmdWatchClear (int args);
BOOL CmdZap (int args);

typedef struct _addrrec
{
	char format[12];
	int   bytes;
} addrrec, *addrptr;

typedef struct _argrec
{
	char str[12];
	WORD  val1;
	WORD  val2;
} argrec, *argptr;

typedef struct _bprec
{
	WORD address;
	WORD length;
	BOOL enabled;
} bprec, *bpptr;

typedef struct _cmdrec
{
	char       name[12];
	cmdfunction function;
} cmdrec, *cmdptr;

typedef struct _instrec
{
	char mnemonic[4];
	int   addrmode;
} instrec, *instptr;

typedef struct _symbolrec
{
	WORD  value;
	char name[14];
} symbolrec, *symbolptr;

addrrec addressmode[17]  = { {""         ,1},        // (implied)
                             {""         ,1},        // INVALID1
                             {""         ,2},        // INVALID2
                             {""         ,3},        // INVALID3
                             {"#$%02X"   ,2},        // ADDR_IMM
                             {"%s"       ,3},        // ADDR_ABS
                             {"%s"       ,2},        // ADDR_ZPG
                             {"%s,X"     ,3},        // ADDR_ABSX
                             {"%s,Y"     ,3},        // ADDR_ABSY
                             {"%s,X"     ,2},        // ADDR_ZPGX
                             {"%s,Y"     ,2},        // ADDR_ZPGY
                             {"%s"       ,2},        // ADDR_REL
                             {"($%02X,X)",2},        // ADDR_INDX
                             {"($%02X),Y",2},        // ADDR_INDY
                             {"($%04X)"  ,3} };      // ADDR_IABS

cmdrec command[COMMANDS] = { {"BA"      ,CmdBreakpointAdd},
                             {"BC"      ,CmdBreakpointClear},
                             {"BD"      ,CmdBreakpointDisable},
                             {"BE"      ,CmdBreakpointEnable},
                             {"BPM"     ,CmdBreakpointAdd},
                             {"BW"      ,CmdBlackWhite},
                             {"CC"      ,CmdCycleClear},
                             {"COLOR"   ,CmdColor},
                             {"D"       ,CmdMemoryDump},
                             {"EXTBENCH",CmdExtBenchmark},
                             {"GOTO"    ,CmdGo},
                             {"I"       ,CmdInput},
                             {"ICD"     ,CmdInternalCodeDump},
                             {"IMD"     ,CmdInternalMemoryDump},
                             {"INPUT"   ,CmdInput},
                             {"KEY"     ,CmdFeedKey},
                             {"MC"      ,NULL}, // CmdMemoryCompare
                             {"MD"      ,CmdMemoryDump},
                             {"MDB"     ,CmdMemoryDump},
                             {"MDC"     ,CmdCodeDump},
                             {"ME"      ,CmdMemoryEnter},
                             {"MEB"     ,CmdMemoryEnter},
                             {"MEMORY"  ,CmdMemoryDump},
                             {"MF"      ,CmdMemoryFill},
                             {"MONO"    ,CmdBlackWhite},
                             {"MS"      ,NULL}, // CmdMemorySearch
                             {"O"       ,CmdOutput},
                             {"P"       ,NULL}, // CmdStep
                             {"PROFILE" ,CmdProfile},
                             {"R"       ,CmdRegisterSet},
                             {"RB"      ,CmdFlagSet},
                             {"RC"      ,CmdFlagSet},
                             {"RD"      ,CmdFlagSet},
                             {"REGISTER",CmdRegisterSet},
                             {"RET"     ,NULL}, // CmdReturn
                             {"RI"      ,CmdFlagSet},
                             {"RN"      ,CmdFlagSet},
                             {"RR"      ,CmdFlagSet},
                             {"RTS"     ,NULL}, // CmdReturn
                             {"RV"      ,CmdFlagSet},
                             {"RZ"      ,CmdFlagSet},
                             {"SB"      ,CmdFlagSet},
                             {"SC"      ,CmdFlagSet},
                             {"SD"      ,CmdFlagSet},
                             {"SI"      ,CmdFlagSet},
                             {"SN"      ,CmdFlagSet},
                             {"SR"      ,CmdFlagSet},
                             {"SV"      ,CmdFlagSet},
                             {"SYM"     ,NULL}, // CmdSymbol
                             {"SZ"      ,CmdFlagSet},
                             {"T"       ,CmdTrace},
							 {"TO"      ,CmdTraceOut},
							 {"TF"      ,CmdTraceFile},
                             {"TL"      ,CmdTraceLine},
                             {"TRACE"   ,CmdTrace},
                             {"U"       ,CmdCodeDump},
                             {"W"       ,CmdWatchAdd},
                             {"W?"      ,CmdWatchAdd},
                             {"WATCH"   ,CmdWatchAdd},
                             {"WC"      ,CmdWatchClear},
                             {"ZAP"     ,CmdZap},
                             {"\\"      ,CmdViewOutput}};

instrec instruction[256] = { {"BRK",0},              // 00h
                             {"ORA",ADDR_INDX},      // 01h
                             {"???",INVALID2},       // 02h
                             {"???",INVALID1},       // 03h
                             {"???",INVALID2},       // 04h
                             {"ORA",ADDR_ZPG},       // 05h
                             {"ASL",ADDR_ZPG},       // 06h
                             {"???",INVALID1},       // 07h
                             {"PHP",0},              // 08h
                             {"ORA",ADDR_IMM},       // 09h
                             {"ASL",0},              // 0Ah
                             {"???",INVALID1},       // 0Bh
                             {"???",INVALID3},       // 0Ch
                             {"ORA",ADDR_ABS},       // 0Dh
                             {"ASL",ADDR_ABS},       // 0Eh
                             {"???",INVALID1},       // 0Fh
                             {"BPL",ADDR_REL},       // 10h
                             {"ORA",ADDR_INDY},      // 11h
                             {"???",INVALID2},       // 12h
                             {"???",INVALID1},       // 13h
                             {"???",INVALID2},       // 14h
                             {"ORA",ADDR_ZPGX},      // 15h
                             {"ASL",ADDR_ZPGX},      // 16h
                             {"???",INVALID1},       // 17h
                             {"CLC",0},              // 18h
                             {"ORA",ADDR_ABSY},      // 19h
                             {"???",INVALID1},       // 1Ah
                             {"???",INVALID1},       // 1Bh
                             {"???",INVALID3},       // 1Ch
                             {"ORA",ADDR_ABSX},      // 1Dh
                             {"ASL",ADDR_ABSX},      // 1Eh
                             {"???",INVALID1},       // 1Fh
                             {"JSR",ADDR_ABS},       // 20h
                             {"AND",ADDR_INDX},      // 21h
                             {"???",INVALID2},       // 22h
                             {"???",INVALID1},       // 23h
                             {"BIT",ADDR_ZPG},       // 24h
                             {"AND",ADDR_ZPG},       // 25h
                             {"ROL",ADDR_ZPG},       // 26h
                             {"???",INVALID1},       // 27h
                             {"PLP",0},              // 28h
                             {"AND",ADDR_IMM},       // 29h
                             {"ROL",0},              // 2Ah
                             {"???",INVALID1},       // 2Bh
                             {"BIT",ADDR_ABS},       // 2Ch
                             {"AND",ADDR_ABS},       // 2Dh
                             {"ROL",ADDR_ABS},       // 2Eh
                             {"???",INVALID1},       // 2Fh
                             {"BMI",ADDR_REL},       // 30h
                             {"AND",ADDR_INDY},      // 31h
                             {"???",INVALID2},       // 32h
                             {"???",INVALID1},       // 33h
                             {"???",INVALID2},       // 34h
                             {"AND",ADDR_ZPGX},      // 35h
                             {"ROL",ADDR_ZPGX},      // 36h
                             {"???",INVALID1},       // 37h
                             {"SEC",0},              // 38h
                             {"AND",ADDR_ABSY},      // 39h
                             {"???",INVALID1},       // 3Ah
                             {"???",INVALID1},       // 3Bh
                             {"???",INVALID3},       // 3Ch
                             {"AND",ADDR_ABSX},      // 3Dh
                             {"ROL",ADDR_ABSX},      // 3Eh
                             {"???",INVALID1},       // 3Fh
                             {"RTI",0},              // 40h
                             {"EOR",ADDR_INDX},      // 41h
                             {"???",INVALID2},       // 42h
                             {"???",INVALID1},       // 43h
                             {"???",INVALID2},       // 44h
                             {"EOR",ADDR_ZPG},       // 45h
                             {"LSR",ADDR_ZPG},       // 46h
                             {"???",INVALID1},       // 47h
                             {"PHA",0},              // 48h
                             {"EOR",ADDR_IMM},       // 49h
                             {"LSR",0},              // 4Ah
                             {"???",INVALID1},       // 4Bh
                             {"JMP",ADDR_ABS},       // 4Ch
                             {"EOR",ADDR_ABS},       // 4Dh
                             {"LSR",ADDR_ABS},       // 4Eh
                             {"???",INVALID1},       // 4Fh
                             {"BVC",ADDR_REL},       // 50h
                             {"EOR",ADDR_INDY},      // 51h
                             {"???",INVALID2},       // 52h
                             {"???",INVALID1},       // 53h
                             {"???",INVALID2},       // 54h
                             {"EOR",ADDR_ZPGX},      // 55h
                             {"LSR",ADDR_ZPGX},      // 56h
                             {"???",INVALID1},       // 57h
                             {"CLI",0},              // 58h
                             {"EOR",ADDR_ABSY},      // 59h
                             {"???",INVALID1},       // 5Ah
                             {"???",INVALID1},       // 5Bh
                             {"???",INVALID3},       // 5Ch
                             {"EOR",ADDR_ABSX},      // 5Dh
                             {"LSR",ADDR_ABSX},      // 5Eh
                             {"???",INVALID1},       // 5Fh
                             {"RTS",0},              // 60h
                             {"ADC",ADDR_INDX},      // 61h
                             {"???",INVALID2},       // 62h
                             {"???",INVALID1},       // 63h
                             {"???",INVALID2},       // 64h
                             {"ADC",ADDR_ZPG},       // 65h
                             {"ROR",ADDR_ZPG},       // 66h
                             {"???",INVALID1},       // 67h
                             {"PLA",0},              // 68h
                             {"ADC",ADDR_IMM},       // 69h
                             {"ROR",0},              // 6Ah
                             {"???",INVALID1},       // 6Bh
                             {"JMP",ADDR_IABS},      // 6Ch
                             {"ADC",ADDR_ABS},       // 6Dh
                             {"ROR",ADDR_ABS},       // 6Eh
                             {"???",INVALID1},       // 6Fh
                             {"BVS",ADDR_REL},       // 70h
                             {"ADC",ADDR_INDY},      // 71h
                             {"???",INVALID2},       // 72h
                             {"???",INVALID1},       // 73h
                             {"???",INVALID2},       // 74h
                             {"ADC",ADDR_ZPGX},      // 75h
                             {"ROR",ADDR_ZPGX},      // 76h
                             {"???",INVALID1},       // 77h
                             {"SEI",0},              // 78h
                             {"ADC",ADDR_ABSY},      // 79h
                             {"???",INVALID1},       // 7Ah
                             {"???",INVALID1},       // 7Bh
                             {"???",INVALID3},       // 7Ch
                             {"ADC",ADDR_ABSX},      // 7Dh
                             {"ROR",ADDR_ABSX},      // 7Eh
                             {"???",INVALID1},       // 7Fh
                             {"???",INVALID2},       // 80h
                             {"STA",ADDR_INDX},      // 81h
                             {"???",INVALID2},       // 82h
                             {"???",INVALID1},       // 83h
                             {"STY",ADDR_ZPG},       // 84h
                             {"STA",ADDR_ZPG},       // 85h
                             {"STX",ADDR_ZPG},       // 86h
                             {"???",INVALID1},       // 87h
                             {"DEY",0},              // 88h
                             {"BIT",ADDR_IMM},       // 89h
                             {"TXA",0},              // 8Ah
                             {"???",INVALID1},       // 8Bh
                             {"STY",ADDR_ABS},       // 8Ch
                             {"STA",ADDR_ABS},       // 8Dh
                             {"STX",ADDR_ABS},       // 8Eh
                             {"???",INVALID1},       // 8Fh
                             {"BCC",ADDR_REL},       // 90h
                             {"STA",ADDR_INDY},      // 91h
                             {"???",INVALID2},       // 92h
                             {"???",INVALID1},       // 93h
                             {"STY",ADDR_ZPGX},      // 94h
                             {"STA",ADDR_ZPGX},      // 95h
                             {"STX",ADDR_ZPGY},      // 96h
                             {"???",INVALID1},       // 97h
                             {"TYA",0},              // 98h
                             {"STA",ADDR_ABSY},      // 99h
                             {"TXS",0},              // 9Ah
                             {"???",INVALID1},       // 9Bh
                             {"???",INVALID3},       // 9Ch
                             {"STA",ADDR_ABSX},      // 9Dh
                             {"???",INVALID3},       // 9Eh
                             {"???",INVALID1},       // 9Fh
                             {"LDY",ADDR_IMM},       // A0h
                             {"LDA",ADDR_INDX},      // A1h
                             {"LDX",ADDR_IMM},       // A2h
                             {"???",INVALID1},       // A3h
                             {"LDY",ADDR_ZPG},       // A4h
                             {"LDA",ADDR_ZPG},       // A5h
                             {"LDX",ADDR_ZPG},       // A6h
                             {"???",INVALID1},       // A7h
                             {"TAY",0},              // A8h
                             {"LDA",ADDR_IMM},       // A9h
                             {"TAX",0},              // AAh
                             {"???",INVALID1},       // ABh
                             {"LDY",ADDR_ABS},       // ACh
                             {"LDA",ADDR_ABS},       // ADh
                             {"LDX",ADDR_ABS},       // AEh
                             {"???",INVALID1},       // AFh
                             {"BCS",ADDR_REL},       // B0h
                             {"LDA",ADDR_INDY},      // B1h
                             {"???",INVALID2},       // B2h
                             {"???",INVALID1},       // B3h
                             {"LDY",ADDR_ZPGX},      // B4h
                             {"LDA",ADDR_ZPGX},      // B5h
                             {"LDX",ADDR_ZPGY},      // B6h
                             {"???",INVALID1},       // B7h
                             {"CLV",0},              // B8h
                             {"LDA",ADDR_ABSY},      // B9h
                             {"TSX",0},              // BAh
                             {"???",INVALID1},       // BBh
                             {"LDY",ADDR_ABSX},      // BCh
                             {"LDA",ADDR_ABSX},      // BDh
                             {"LDX",ADDR_ABSY},      // BEh
                             {"???",INVALID1},       // BFh
                             {"CPY",ADDR_IMM},       // C0h
                             {"CMP",ADDR_INDX},      // C1h
                             {"???",INVALID2},       // C2h
                             {"???",INVALID1},       // C3h
                             {"CPY",ADDR_ZPG},       // C4h
                             {"CMP",ADDR_ZPG},       // C5h
                             {"DEC",ADDR_ZPG},       // C6h
                             {"???",INVALID1},       // C7h
                             {"INY",0},              // C8h
                             {"CMP",ADDR_IMM},       // C9h
                             {"DEX",0},              // CAh
                             {"???",INVALID1},       // CBh
                             {"CPY",ADDR_ABS},       // CCh
                             {"CMP",ADDR_ABS},       // CDh
                             {"DEC",ADDR_ABS},       // CEh
                             {"???",INVALID1},       // CFh
                             {"BNE",ADDR_REL},       // D0h
                             {"CMP",ADDR_INDY},      // D1h
                             {"???",INVALID2},       // D2h
                             {"???",INVALID1},       // D3h
                             {"???",INVALID2},       // D4h
                             {"CMP",ADDR_ZPGX},      // D5h
                             {"DEC",ADDR_ZPGX},      // D6h
                             {"???",INVALID1},       // D7h
                             {"CLD",0},              // D8h
                             {"CMP",ADDR_ABSY},      // D9h
                             {"???",INVALID1},       // DAh
                             {"???",INVALID1},       // DBh
                             {"???",INVALID3},       // DCh
                             {"CMP",ADDR_ABSX},      // DDh
                             {"DEC",ADDR_ABSX},      // DEh
                             {"???",INVALID1},       // DFh
                             {"CPX",ADDR_IMM},       // E0h
                             {"SBC",ADDR_INDX},      // E1h
                             {"???",INVALID2},       // E2h
                             {"???",INVALID1},       // E3h
                             {"CPX",ADDR_ZPG},       // E4h
                             {"SBC",ADDR_ZPG},       // E5h
                             {"INC",ADDR_ZPG},       // E6h
                             {"???",INVALID1},       // E7h
                             {"INX",0},              // E8h
                             {"SBC",ADDR_IMM},       // E9h
                             {"NOP",0},              // EAh
                             {"???",INVALID1},       // EBh
                             {"CPX",ADDR_ABS},       // ECh
                             {"SBC",ADDR_ABS},       // EDh
                             {"INC",ADDR_ABS},       // EEh
                             {"???",INVALID1},       // EFh
                             {"BEQ",ADDR_REL},       // F0h
                             {"SBC",ADDR_INDY},      // F1h
                             {"???",INVALID2},       // F2h
                             {"???",INVALID1},       // F3h
                             {"???",INVALID2},       // F4h
                             {"SBC",ADDR_ZPGX},      // F5h
                             {"INC",ADDR_ZPGX},      // F6h
                             {"???",INVALID1},       // F7h
                             {"SED",0},              // F8h
                             {"SBC",ADDR_ABSY},      // F9h
                             {"???",INVALID1},       // FAh
                             {"???",INVALID1},       // FBh
                             {"???",INVALID3},       // FCh
                             {"SBC",ADDR_ABSX},      // FDh
                             {"INC",ADDR_ABSX},      // FEh
                             {"???",INVALID1} };     // FFh

DWORD color[2][_COLORS] = {{0x800000,0xC0C0C0,0x00FFFF,0x808000,
                              0x000080,0x800000,0x00FFFF,0xFFFFFF},
                             {0x000000,0xC0C0C0,0xFFFFFF,0x000000,
                              0xC0C0C0,0x808080,0xFFFFFF,0xFFFFFF}};

char commandstring[COMMANDLINES][80] = {"",
                                         " ",
                                         " Emulador TK2000 para Windows",
                                         " ",
                                         " "};

argrec arg[MAXARGS];
bprec  breakpoint[BREAKPOINTS];
DWORD  profiledata[256];
int    watch[WATCHES];

int       colorscheme   = 0;
HFONT     debugfont     = (HFONT)0;
DWORD     extbench      = 0;
BOOL      fulldisp      = 0;
WORD      lastpc        = 0;
WORD      memorydump    = 0;
BOOL      profiling     = 0;
int       stepcount     = 0;
BOOL      stepline      = 0;
int       stepstart     = 0;
int       stepuntil     = -1;
symbolptr symboltable   = NULL;
int       symbolnum     = 0;
WORD      topoffset     = 0;
FILE     *tracefile     = NULL;
BOOL      usingbp       = 0;
BOOL      usingmemdump  = 0;
BOOL      usingwatches  = 0;
BOOL      viewingoutput = 0;
unsigned __int64 g_dCumulativeCycles = 0;

// Protótipos
void ComputeTopOffset (WORD centeroffset);
BOOL DisplayError (LPCTSTR errortext);
BOOL DisplayHelp (cmdfunction function);
BOOL InternalSingleStep ();
WORD GetAddress (LPCTSTR symbol);
LPCTSTR GetSymbol (WORD address, int bytes);
void GetTargets (int *intermediate, int *final);

//===========================================================================
BOOL CheckBreakpoint (WORD address, BOOL memory)
{
	int target[3] = {address,-1,-1};
	int targetnum;

	if (memory)
		GetTargets(&target[1],&target[2]);
	targetnum = memory ? 3 : 1;
	while (targetnum--)
		if (target[targetnum] >= 0)
		{
			WORD targetaddr = (WORD)(target[targetnum] & 0xFFFF);
			int  slot       = 0;
			while (slot < BREAKPOINTS)
			{
				if (breakpoint[slot].length && breakpoint[slot].enabled &&
						(breakpoint[slot].address <= targetaddr) &&
						(breakpoint[slot].address+breakpoint[slot].length > targetaddr))
					return 1;
				slot++;
			}
		}
	return 0;
}

//===========================================================================
BOOL CheckJump (WORD targetaddress) {
	WORD savedpc = regs.pc;
	unsigned __int64 saved_cc1 = g_nCumulativeCycles;
	unsigned __int64 saved_cc2 = g_dCumulativeCycles;
	DWORD saved_cc3            = cumulativecycles;
	BOOL result;

	InternalSingleStep();
	result = (regs.pc == targetaddress);
	regs.pc = savedpc;
	g_nCumulativeCycles = saved_cc1;
	g_dCumulativeCycles = saved_cc2;
	cumulativecycles    = saved_cc3;
	return result;
}

//===========================================================================
BOOL CmdBlackWhite (int args)
{
	colorscheme = 1;
	DebugDisplay(1);
	return 0;
}

//===========================================================================
BOOL CmdBreakpointAdd (int args)
{
	BOOL addedone = 0;
	int  loop     = 0;

	if (!args)
		arg[args = 1].val1 = regs.pc;
	while (loop++ < args)
		if (arg[loop].val1 || (arg[loop].str[0] == '0') ||
				GetAddress(arg[loop].str))
		{
			int freeslot = 0;

			if (!arg[loop].val1)
				arg[loop].val1 = GetAddress(arg[loop].str);

			// FIND A FREE SLOT FOR THIS NEW BREAKPOINT
			while ((freeslot < BREAKPOINTS) && breakpoint[freeslot].length)
				freeslot++;
			if ((freeslot >= BREAKPOINTS) && !addedone)
				return DisplayError("Todos os slots de breakpointsestão em uso.");

			// ADD THE BREAKPOINT
			if (freeslot < BREAKPOINTS)
			{
				breakpoint[freeslot].address = arg[loop].val1;
				breakpoint[freeslot].length  = arg[loop].val2
												? MIN(0x10000-arg[loop].val1,arg[loop].val2)
												: 1;
				breakpoint[freeslot].enabled = 1;
				addedone = 1;
				usingbp  = 1;
			}
		}
	if (!addedone)
		return DisplayHelp(CmdBreakpointAdd);
	return 1;
}

//===========================================================================
BOOL CmdBreakpointClear (int args)
{
	int usedslot = 0;

	// CHECK FOR ERRORS
	if (!args)
		return DisplayHelp(CmdBreakpointClear);
	if (!usingbp)
		return DisplayError("Não há breakpoints definidos.");

	// CLEAR EACH BREAKPOINT IN THE LIST
	while (args)
	{
		if (!strcmp(arg[args].str,"*"))
		{
			int loop = BREAKPOINTS;
			while (loop--)
				breakpoint[loop].length = 0;
		}
		else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
			breakpoint[arg[args].val1-1].length = 0;
		args--;
	}

	// IF THERE ARE NO MORE BREAKPOINTS DEFINED, DISABLE THE BREAKPOINT
	// FUNCTIONALITY AND ERASE THE BREAKPOINT DISPLAY FROM THE SCREEN
	while ((usedslot < BREAKPOINTS) && !breakpoint[usedslot].length)
		usedslot++;
	if (usedslot >= BREAKPOINTS)
	{
		usingbp = 0;
		DebugDisplay(1);
		return 0;
	}
	else
		return 1;
}

//===========================================================================
BOOL CmdBreakpointDisable (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return DisplayHelp(CmdBreakpointDisable);
  if (!usingbp)
    return DisplayError("Não há breakpoints definidos.");

  // DISABLE EACH BREAKPOINT IN THE LIST
  while (args) {
    if (!strcmp(arg[args].str,"*")) {
      int loop = BREAKPOINTS;
      while (loop--)
        breakpoint[loop].enabled = 0;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
      breakpoint[arg[args].val1-1].enabled = 0;
    args--;
  }

  return 1;
}

//===========================================================================
BOOL CmdBreakpointEnable (int args) {

  // CHECK FOR ERRORS
  if (!args)
    return DisplayHelp(CmdBreakpointEnable);
  if (!usingbp)
    return DisplayError("Não há breakpoints definidos.");

  // ENABLE EACH BREAKPOINT IN THE LIST
  while (args) {
    if (!strcmp(arg[args].str,"*")) {
      int loop = BREAKPOINTS;
      while (loop--)
        breakpoint[loop].enabled = 1;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= BREAKPOINTS))
      breakpoint[arg[args].val1-1].enabled = 1;
    args--;
  }

  return 1;
}

//===========================================================================
BOOL CmdCycleClear (int args) {
	g_dCumulativeCycles = 0;
	return 1;
}

//===========================================================================
BOOL CmdCodeDump (int args) {
  if ((!args) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return DisplayHelp(CmdCodeDump);
  topoffset = arg[1].val1;
  if (!topoffset)
    topoffset = GetAddress(arg[1].str);
  return 1;
}

//===========================================================================
BOOL CmdColor (int args) {
  colorscheme = 0;
  DebugDisplay(1);
  return 0;
}

//===========================================================================
BOOL CmdExtBenchmark (int args) {
  DWORD currtime;
  DebugEnd();
  mode = MODE_RUNNING;
  FrameRefreshStatus(DRAW_TITLE);
  VideoRedrawScreen();
  currtime = GetTickCount();
  while ((extbench = GetTickCount()) != currtime) ;
//  KeybQueueKeypress(' ',1);
  resettiming = 1;
  return 0;
}

//===========================================================================
BOOL CmdFeedKey (int args)
{
//	KeybQueueKeypress(args ? arg[1].val1 ? arg[1].val1
//					: arg[1].str[0]
//					: ' ',
//					1);
	return 0;
}

//===========================================================================
BOOL CmdFlagSet (int args) {
  static const char flagname[] = "CZIDBRVN";
  int loop = 0;
  while (loop < 8)
    if (flagname[loop] == arg[0].str[1])
      break;
    else
      loop++;
  if (loop < 8)
    if (arg[0].str[0] == 'R')
      regs.ps &= ~(1 << loop);
    else
      regs.ps |= (1 << loop);
  return 1;
}

//===========================================================================
BOOL CmdGo (int args)
{
	MSG Mensagem;

	stepcount = -1;
	stepline  = 0;
	stepstart = regs.pc;
	stepuntil = args ? arg[1].val1 : -1;
	if (!stepuntil)
		stepuntil = GetAddress(arg[1].str);
	mode = MODE_STEPPING;
	FrameRefreshStatus(DRAW_TITLE);
	GetMessage(&Mensagem,(HWND)framewindow,0,0); // evitar duplo enter
	return 0;
}

//===========================================================================
BOOL CmdInput (int args) {
  if ((!args) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return DisplayHelp(CmdInput);
  if (!arg[1].val1)
    arg[1].val1 = GetAddress(arg[1].str);
  ioread[arg[1].val1 & 0xFF](regs.pc,arg[1].val1 & 0xFF,0,0);
  return 1;
}

//===========================================================================
BOOL CmdInternalCodeDump (int args)
{
	char filename[MAX_PATH];
	HANDLE file;

	if ((!args) ||
			((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
		return DisplayHelp(CmdInternalCodeDump);
	if (!arg[1].val1)
		arg[1].val1 = GetAddress(arg[1].str);
	strcpy(filename,progdir);
	strcat(filename,"Output.bin");
	file = CreateFile(filename,
							GENERIC_WRITE,
							0,
							(LPSECURITY_ATTRIBUTES)NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
							NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD codelength = 0;
		char* codeptr    = NULL;
		//CpuGetCode(arg[1].val1,&codeptr,&codelength);
		if (codeptr && codelength)
		{
			DWORD byteswritten = 0;
			WriteFile(file,codeptr,codelength,&byteswritten,NULL);
		}
		CloseHandle(file);
	}
	return 0;
}

//===========================================================================
BOOL CmdInternalMemoryDump (int args)
{
/*
	char filename[MAX_PATH];
	HANDLE file;

	strcpy(filename,progdir);
	strcat(filename,"Output.bin");
	file = CreateFile(filename,
							GENERIC_WRITE,
							0,
							(LPSECURITY_ATTRIBUTES)NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
							NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD byteswritten;
		WriteFile(file,
				mem,
				0x30000,
				&byteswritten,
				NULL);
		CloseHandle(file);
	}
*/
	return 0;
}

//===========================================================================
BOOL CmdLineDown (int args)
 {
  topoffset += addressmode[instruction[mem_readb(topoffset, 0)].addrmode].bytes;
  return 1;
}

//===========================================================================
BOOL CmdLineUp (int args) {
  WORD newoffset;
  WORD savedoffset = topoffset;

  ComputeTopOffset(topoffset);
  newoffset = topoffset;
  while (newoffset < savedoffset) {
    topoffset  = newoffset;
    newoffset += addressmode[instruction[mem_readb(newoffset, 0)].addrmode].bytes;
  }
  topoffset = MIN(topoffset,savedoffset-1);
  return 1;
}

//===========================================================================
BOOL CmdMemoryDump (int args) {
  if ((!args) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return DisplayHelp(CmdMemoryDump);
  memorydump   = arg[1].val1;
  if (!memorydump)
    memorydump = GetAddress(arg[1].str);
  usingmemdump = 1;
  return 1;
}

//===========================================================================
BOOL CmdMemoryEnter (int args) {
  WORD address;
  if ((args < 2) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))) ||
      ((arg[2].str[0] != '0') && !arg[2].val1))
    return DisplayHelp(CmdMemoryEnter);
  address = arg[1].val1;
  if (!address)
    address = GetAddress(arg[1].str);
  while (args >= 2) {
    mem_writeb(address+args-2, (BYTE)arg[args].val1);
    *(memdirty+(address >> 8)) = 1;
    args--;
  }
  return 1;
}

//===========================================================================
BOOL CmdMemoryFill (int args) {
  WORD address;
  WORD bytes;

  if ((!args) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return DisplayHelp(CmdMemoryFill);
  address = arg[1].val1 ? arg[1].val1 : GetAddress(arg[1].str);
  bytes   = MAX(1,arg[1].val2);
  while (bytes--) {
    if ((address < 0xC000) || (address > 0xC0FF))
      mem_writeb(address, (BYTE)(arg[2].val1 & 0xFF));
    address++;
  }
  return 1;
}

//===========================================================================
BOOL CmdOutput (int args) {
  if ((!args) ||
      ((arg[1].str[0] != '0') && (!arg[1].val1) && (!GetAddress(arg[1].str))))
    return DisplayHelp(CmdInput);
  if (!arg[1].val1)
    arg[1].val1 = GetAddress(arg[1].str);
  iowrite[arg[1].val1 & 0xFF](regs.pc,arg[1].val1 & 0xFF,1,arg[2].val1 & 0xFF);
  return 1;
}

//===========================================================================
BOOL CmdPageDown (int args) {
  int loop = 0;
  while (loop++ < SOURCELINES)
    CmdLineDown(args);
  return 1;
}

//===========================================================================
BOOL CmdPageUp (int args) {
  int loop = 0;
  while (loop++ < SOURCELINES)
    CmdLineUp(args);
  return 1;
}

//===========================================================================
BOOL CmdProfile (int args) {
  ZeroMemory(profiledata,256*sizeof(DWORD));
  profiling = 1;
  return 0;
}

//===========================================================================
BOOL CmdRegisterSet (int args) {
  if ((args == 2) &&
      (arg[1].str[0] == 'P') && (arg[2].str[0] == 'L'))
    regs.pc = lastpc;
  else if ((args < 2) || ((arg[2].str[0] != '0') && !arg[2].val1))
    return DisplayHelp(CmdMemoryEnter);
  else switch (arg[1].str[0]) {
    case 'A': regs.a  = (BYTE)(arg[2].val1 & 0xFF);    break;
    case 'P': regs.pc = arg[2].val1;                   break;
    case 'S': regs.sp = 0x100 | (arg[2].val1 & 0xFF);  break;
    case 'X': regs.x  = (BYTE)(arg[2].val1 & 0xFF);    break;
    case 'Y': regs.y  = (BYTE)(arg[2].val1 & 0xFF);    break;
    default:        return DisplayHelp(CmdMemoryEnter);
  }
  ComputeTopOffset(regs.pc);
  return 1;
}

//===========================================================================
BOOL CmdTrace (int args)
{
	stepcount = args ? arg[1].val1 : 1;
	stepline  = 0;
	stepstart = regs.pc;
	stepuntil = -1;
	mode = MODE_STEPPING;
	FrameRefreshStatus(DRAW_TITLE);
	DebugContinueStepping();
	return 0;
}

//===========================================================================
BOOL CmdTraceOut (int args)
{
	MSG Mensagem;
	BYTE inst = mem_readb(regs.pc, 0);
	stepcount = (inst == 0x20) ? -1 : 1;
	stepline  = 0;
	stepstart = regs.pc;
	stepuntil = (inst == 0x20) ? regs.pc+3 : -1;
	mode = MODE_STEPPING;
	FrameRefreshStatus(DRAW_TITLE);
	GetMessage(&Mensagem,(HWND)framewindow,0,0); // evitar duplo enter
	return 0;
}


//===========================================================================
BOOL CmdTraceFile (int args) {
  char filename[MAX_PATH];

  if (tracefile)
    fclose(tracefile);
  strcpy(filename,progdir);
  strcat(filename,(args && arg[1].str[0]) ? arg[1].str : "Trace.txt");
  tracefile = fopen(filename,"wt");
  return 0;
}

//===========================================================================
BOOL CmdTraceLine (int args) {
  stepcount = args ? arg[1].val1 : 1;
  stepline  = 1;
  stepstart = regs.pc;
  stepuntil = -1;
  mode = MODE_STEPPING;
  FrameRefreshStatus(DRAW_TITLE);
  DebugContinueStepping();
  return 0;
}

//===========================================================================
BOOL CmdViewOutput (int args) {
	VideoRedrawScreen();
	viewingoutput = 1;
	return 0;
}

//===========================================================================
BOOL CmdWatchAdd (int args) {
  BOOL addedone = 0;
  int  loop     = 0;

  if (!args)
    return DisplayHelp(CmdWatchAdd);
  while (loop++ < args)
    if (arg[loop].val1 || (arg[loop].str[0] == '0') ||
        GetAddress(arg[loop].str)) {
      int freeslot = 0;

      if (!arg[loop].val1)
        arg[loop].val1 = GetAddress(arg[loop].str);

      // FIND A FREE SLOT FOR THIS NEW WATCH
      while ((freeslot < WATCHES) && (watch[freeslot] >= 0))
        freeslot++;
      if ((freeslot >= WATCHES) && !addedone)
        return DisplayError("Todos os slots de Watch estão em uso.");

      // VERIFY THAT THE WATCH IS NOT ON AN I/O LOCATION
      if ((arg[loop].val1 >= 0xC000) && (arg[loop].val1 <= 0xC0FF))
        return DisplayError("Você não pode ter um Watch em um endereço de I/O.");

      // ADD THE WATCH
      if (freeslot < WATCHES) {
        watch[freeslot] = arg[loop].val1;
        addedone        = 1;
        usingwatches    = 1;
      }

    }
  if (!addedone)
    return DisplayHelp(CmdWatchAdd);
  return 1;
}

//===========================================================================
BOOL CmdWatchClear (int args) {
  int usedslot = 0;

  // CHECK FOR ERRORS
  if (!args)
    return DisplayHelp(CmdWatchAdd);
  if (!usingwatches)
    return DisplayError("Não há Watches definidos.");

  // CLEAR EACH WATCH IN THE LIST
  while (args) {
    if (!strcmp(arg[args].str,"*")) {
      int loop = WATCHES;
      while (loop--)
        watch[loop] = -1;
    }
    else if ((arg[args].val1 >= 1) && (arg[args].val1 <= WATCHES))
      watch[arg[args].val1-1] = -1;
    args--;
  }

  // IF THERE ARE NO MORE WATCHES DEFINED, ERASE THE WATCH DISPLAY
  while ((usedslot < WATCHES) && (watch[usedslot] < 0))
    usedslot++;
  if (usedslot >= WATCHES) {
    usingwatches = 0;
    DebugDisplay(1);
    return 0;
  }
  else
    return 1;

}

//===========================================================================
BOOL CmdZap (int args) {
  int loop = addressmode[instruction[mem_readb(regs.pc, 0)].addrmode].bytes;
  while (loop--)
    mem_writeb(regs.pc+loop, 0xEA);
  return 1;
}

//===========================================================================
void ComputeTopOffset (WORD centeroffset) {
  BOOL invalid;

  topoffset = centeroffset - 0x30;
  do {
    WORD instofs[0x30];
    WORD currofs = topoffset;
    int  currnum = 0;

    invalid = 0;
    do {
      int addrmode = instruction[mem_readb(currofs, 0)].addrmode;
      if ((addrmode >= 1) && (addrmode <= 3))
        invalid = 1;
      else {
        instofs[currnum++] = currofs;
        currofs += addressmode[addrmode].bytes;
      }
    } while ((!invalid) && (currofs < centeroffset));
    if (invalid)
      topoffset++;
    else if (currnum > (SOURCELINES >> 1))
      topoffset = instofs[currnum-(SOURCELINES >> 1)];
  } while (invalid);
}

//===========================================================================
BOOL DisplayError (LPCTSTR errortext) {
  return 0;
}

//===========================================================================
BOOL DisplayHelp (cmdfunction function) {
  return 0;
}

//===========================================================================
void DrawBreakpoints (HDC dc, int line) {
  RECT linerect;
  char fulltext[16] = "Breakpoints";
  int loop = 0;

  linerect.left   = SCREENSPLIT2;
  linerect.top    = (line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
  SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
  do {
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               fulltext,strlen(fulltext),NULL);
    linerect.top    += 16;
    linerect.bottom += 16;
    if ((loop < BREAKPOINTS) && breakpoint[loop].length) {
      wsprintf(fulltext,
               "%d: %04X",
               loop+1,
               (unsigned)breakpoint[loop].address);
      if (breakpoint[loop].length > 1)
        wsprintf(fulltext+strlen(fulltext),
                 "-%04X",
                 (unsigned)(breakpoint[loop].address+breakpoint[loop].length-1));
      SetTextColor(dc,color[colorscheme][breakpoint[loop].enabled ? COLOR_BPDATA
                                                                  : COLOR_STATIC]);
    }
    else
      fulltext[0] = 0;
  } while (loop++ < BREAKPOINTS);
}

//===========================================================================
void DrawCommandLine (HDC dc, int line) {
  RECT linerect;
  BOOL title = (commandstring[line][0] == ' ');

  SetTextColor(dc,color[colorscheme][COLOR_COMMAND]);
  SetBkColor(dc,0);
  linerect.left   = 0;
  linerect.top    = 368-(line << 4);
  linerect.right  = 12;
  linerect.bottom = linerect.top+16;
  if (!title) {
    ExtTextOut(dc,1,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               ">",1,NULL);
    linerect.left = 12;
  }
  linerect.right = 560;
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             commandstring[line]+title,strlen(commandstring[line]+title),NULL);
}

//===========================================================================
WORD DrawDisassembly (HDC dc, int line, WORD offset, LPTSTR text) {
	char addresstext[40] = "";
	char bytestext[10]   = "";
	char fulltext[50]    = "";
	BYTE  inst            = mem_readb(offset, 0);
	int   addrmode        = instruction[inst].addrmode;
	WORD  bytes           = addressmode[addrmode].bytes;

	// BUILD A STRING CONTAINING THE TARGET ADDRESS OR SYMBOL
	if (addressmode[addrmode].format[0]) {
		WORD address = mem_readw(offset+1, 0);
		if (bytes == 2)
			address &= 0xFF;
		if (addrmode == ADDR_REL)
			address = offset+2+(int)(signed char)address;
		if (strstr(addressmode[addrmode].format,"%s"))
			wsprintf(addresstext,
					addressmode[addrmode].format,
					(LPCTSTR)GetSymbol(address,bytes));
		else
			wsprintf(addresstext,
					addressmode[addrmode].format,
					(unsigned)address);
		if ((addrmode == ADDR_REL) && (offset == regs.pc) && CheckJump(address)) {
			if (address > offset)
				strcat(addresstext," \xCB");		// Seta para baixo AF
			else
				strcat(addresstext," \xCA");		// Seta para cima AE
		}
	}

	// BUILD A STRING CONTAINING THE ACTUAL BYTES THAT MAKE UP THIS
	// INSTRUCTION
	{
	int loop = 0;
	while (loop < bytes)
		wsprintf(bytestext+strlen(bytestext),
				"%02X",
				(unsigned)mem_readb(offset+(loop++), 0));
	while (strlen(bytestext) < 6)
		strcat(bytestext," ");
	}

	// PUT TOGETHER ALL OF THE DIFFERENT ELEMENTS THAT WILL MAKE UP THE LINE
	wsprintf(fulltext,
			"%04X  %s  %-9s %s %s",
			(unsigned)offset,
			(LPCTSTR)bytestext,
			(LPCTSTR)GetSymbol(offset,0),
			(LPCTSTR)instruction[inst].mnemonic,
			(LPCTSTR)addresstext);
	if (text)
	strcpy(text,fulltext);

	// DRAW THE LINE
	if (dc) {
	RECT linerect;
	BOOL bp;

	linerect.left   = 0;
	linerect.top    = line << 4;
	linerect.right  = SCREENSPLIT1-14;
	linerect.bottom = linerect.top+16;
	bp = usingbp && CheckBreakpoint(offset,offset == regs.pc);
	SetTextColor(dc,color[colorscheme][(offset == regs.pc) ? COLOR_INSTBKG
															: bp ? COLOR_INSTBP
																: COLOR_INSTTEXT]);
	SetBkColor(dc,color[colorscheme][(offset == regs.pc) ? bp ? COLOR_INSTBP
																: COLOR_INSTTEXT
															: COLOR_INSTBKG]);
	ExtTextOut(dc,12,linerect.top,
				ETO_CLIPPED | ETO_OPAQUE,&linerect,
				fulltext,strlen(fulltext),NULL);
	}

	return bytes;
}

//===========================================================================
void DrawFlags (HDC dc, int line, WORD value, LPTSTR text) {
  char mnemonic[9] = "NVRBDIZC";
  char fulltext[2] = "?";
  RECT  linerect;
  int loop;

  if (dc) {
    linerect.left   = SCREENSPLIT1+63;
    linerect.top    = line << 4;
    linerect.right  = SCREENSPLIT1+72;
    linerect.bottom = linerect.top+16;
    SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
  }
  loop = 8;
  while (loop--) {
    if (dc) {
      fulltext[0] = mnemonic[loop];
      SetTextColor(dc,color[colorscheme][(value & 1) ? COLOR_DATATEXT
                                                     : COLOR_STATIC]);
      ExtTextOut(dc,linerect.left,linerect.top,
                 ETO_CLIPPED | ETO_OPAQUE,&linerect,
                 fulltext,1,NULL);
      linerect.left  -= 9;
      linerect.right -= 9;
    }
    if (!(value & 1))
      mnemonic[loop] = '.';
    value >>= 1;
  }
  if (text)
    strcpy(text,mnemonic);
}

//===========================================================================
void DrawMemory (HDC dc, int line) {
  RECT linerect;
  char fulltext[16];
  WORD curraddr;
  int  loop = 0;

  linerect.left   = SCREENSPLIT2;
  linerect.top    = (line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  wsprintf(fulltext,"Mem em %04X",(unsigned)memorydump);
  SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
  SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
  curraddr = memorydump;
  do {
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               fulltext,strlen(fulltext),NULL);
    linerect.top    += 16;
    linerect.bottom += 16;
    fulltext[0] = 0;
    if (loop < 4) {
      int loop2 = 0;
      while (loop2++ < 4) {
        if ((curraddr >= 0xC000) && (curraddr <= 0xC0FF))
          strcpy(fulltext+strlen(fulltext),"IO ");
        else
          wsprintf(fulltext+strlen(fulltext),
                   "%02X ",
                   (unsigned)mem_readb(curraddr, 0));
        curraddr++;
      }
    }
    SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
  } while (loop++ < 4);
}

//===========================================================================
void DrawRegister (HDC dc, int line, LPCTSTR name, int bytes, WORD value) {
  RECT linerect;
  char valuestr[8];

  linerect.left   = SCREENSPLIT1;
  linerect.top    = line << 4;
  linerect.right  = SCREENSPLIT1+40;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
  SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             name,strlen(name),NULL);
  if (bytes == 2)
    wsprintf(valuestr,"%04X",(unsigned)value);
  else
    wsprintf(valuestr,"%02X",(unsigned)value);
  linerect.left  = SCREENSPLIT1+40;
  linerect.right = SCREENSPLIT2;
  SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             valuestr,strlen(valuestr),NULL);
}

//===========================================================================
void DrawStack (HDC dc, int line) {
  unsigned curraddr = regs.sp;
  int      loop     = 0;
  while (loop < STACKLINES) {
    RECT linerect;
    char outtext[8] = "";

    curraddr++;
    linerect.left   = SCREENSPLIT1;
    linerect.top    = (loop+line) << 4;
    linerect.right  = SCREENSPLIT1+40;
    linerect.bottom = linerect.top+16;
    SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
    SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
    if (curraddr <= 0x1FF)
      wsprintf(outtext,"%04X",curraddr);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outtext,strlen(outtext),NULL);
    linerect.left   = SCREENSPLIT1+40;
    linerect.right  = SCREENSPLIT2;
    SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
    if (curraddr <= 0x1FF)
      wsprintf(outtext,"%02X",(unsigned)mem_readb(curraddr, 0));
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outtext,strlen(outtext),NULL);
    loop++;
  }
}

//===========================================================================
void DrawTargets (HDC dc, int line) {
  int address[2];
  int loop = 2;

  GetTargets(&address[0],&address[1]);
  while (loop--) {
    char addressstr[8] = "";
    char valuestr[8]   = "";
    RECT linerect;

    if ((address[loop] >= 0xC000) && (address[loop] <= 0xC0FF))
      address[loop] = -1;
    if (address[loop] >= 0) {
      wsprintf(addressstr,"%04X",address[loop]);
      if (loop)
        wsprintf(valuestr,"%02X",mem_readb(address[loop], 0));
      else
        wsprintf(valuestr,"%04X",mem_readw(address[loop], 0));
    }
    linerect.left   = SCREENSPLIT1;
    linerect.top    = (line+loop) << 4;
    linerect.right  = SCREENSPLIT1+40;
    linerect.bottom = linerect.top+16;
    SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
    SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               addressstr,strlen(addressstr),NULL);
    linerect.left  = SCREENSPLIT1+40;
    linerect.right = SCREENSPLIT2;
    SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               valuestr,strlen(valuestr),NULL);
  }
}

//===========================================================================
void DrawWatches (HDC dc, int line) {
  RECT linerect;
  char outstr[16] = "Watches";
  int loop = 0;

  linerect.left   = SCREENSPLIT2;
  linerect.top    = (line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
  SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
  ExtTextOut(dc,linerect.left,linerect.top,
             ETO_CLIPPED | ETO_OPAQUE,&linerect,
             outstr,strlen(outstr),NULL);
  linerect.right = SCREENSPLIT2+64;
  while (loop < WATCHES) {
    if (watch[loop] >= 0)
      wsprintf(outstr,"%d: %04X",loop+1,watch[loop]);
    else
      outstr[0] = 0;
    linerect.top    += 16;
    linerect.bottom += 16;
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outstr,strlen(outstr),NULL);
    loop++;
  }
  linerect.left   = SCREENSPLIT2+64;
  linerect.top    = (line << 4);
  linerect.right  = 560;
  linerect.bottom = linerect.top+16;
  SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
  loop = 0;
  while (loop < WATCHES) {
    if (watch[loop] >= 0)
      wsprintf(outstr,"%02X",(unsigned)mem_readb(watch[loop], 0));
    else
      outstr[0] = 0;
    linerect.top    += 16;
    linerect.bottom += 16;
    ExtTextOut(dc,linerect.left,linerect.top,
               ETO_CLIPPED | ETO_OPAQUE,&linerect,
               outstr,strlen(outstr),NULL);
    loop++;
  }
}

//===========================================================================
BOOL ExecuteCommand (int args) {
  LPTSTR name = strtok(commandstring[0]," ,-=");
  int         found    = 0;
  cmdfunction function = NULL;
  int         length;
  int         loop     = 0;

  if (!name)
    name = commandstring[0];
  length = strlen(name);
  while ((loop < COMMANDS) && (name[0] >= command[loop].name[0])) {
    if (!strncmp(name,command[loop].name,length)) {
      function = command[loop].function;
      if (!strcmp(name,command[loop].name)) {
        found = 1;
        loop  = COMMANDS;
      }
      else
        found++;
    }
    loop++;
  }
  if (found > 1)
    return DisplayError("Comando âmbíguo");
  else if (function)
    return function(args);
  else
    return DisplayError("Comando Ilegal");
}

//===========================================================================
void FreeSymbolTable () {
  if (symboltable)
    VirtualFree(symboltable,0,MEM_RELEASE);
  symbolnum   = 0;
  symboltable = NULL;
}

//===========================================================================
WORD GetAddress (LPCTSTR symbol) {
  int loop = symbolnum;
  while (loop--)
    if (!stricmp(symboltable[loop].name,symbol))
      return symboltable[loop].value;
  return 0;
}

//===========================================================================
LPCTSTR GetSymbol (WORD address, int bytes) {
  static char buffer[8];

  // PERFORM A BINARY SEARCH THROUGH THE SYMBOL TABLE LOOKING FOR A VALUE
  // MATCHING THIS ADDRESS
  {
    int lowlimit  = -1;
    int highlimit = symbolnum;
    int curr      = symbolnum >> 1;
	if (!symboltable) return NULL;
    do {
      int diff = ((int)address)-((int)symboltable[curr].value);
      if (diff < 0) {
        highlimit = curr;
        curr = lowlimit+((curr-lowlimit) >> 1);
      }
      else if (diff > 0) {
        lowlimit = curr;
        curr = curr+((highlimit+1-curr) >> 1);
      }
      else
        return symboltable[curr].name;
    } while ((curr > lowlimit) && (curr < highlimit));
  }

  // IF THERE IS NO SYMBOL FOR THIS ADDRESS, THEN JUST RETURN A STRING
  // CONTAINING THE ADDRESS NUMBER
  switch (bytes) {
    case 2:   wsprintf(buffer,"$%02X",(unsigned)address);  break;
    case 3:   wsprintf(buffer,"$%04X",(unsigned)address);  break;
    default:  buffer[0] = 0;                                     break;
  }
  return buffer;

}

//===========================================================================
void GetTargets (int *intermediate, int *final) {
  int  addrmode   = instruction[mem_readb(regs.pc, 0)].addrmode;
  BYTE argument8  = mem_readb(regs.pc+1, 0);
  WORD argument16 = mem_readw(regs.pc+1, 0);

  *intermediate = -1;
  *final        = -1;
  switch (addrmode) {

    case ADDR_ABS:
      *final = argument16;
      break;
/*
    case ADDR_ABSIINDX:
      argument16   += regs.x;
      *intermediate = argument16;
      *final        = mem_readw(*intermediate, 0);
      break;
*/
    case ADDR_ABSX:
      argument16  += regs.x;
      *final       = argument16;
      break;

    case ADDR_ABSY:
      argument16  += regs.y;
      *final       = argument16;
      break;

    case ADDR_IABS:
      *intermediate = argument16;
      *final        = mem_readw(*intermediate, 0);
      break;

    case ADDR_INDX:
      argument8    += regs.x;
      *intermediate = argument8;
      *final        = mem_readw(*intermediate, 0);
      break;

    case ADDR_INDY:
      *intermediate = argument8;
      *final        = (mem_readw(*intermediate, 0))+regs.y;
      break;
/*
    case ADDR_IZPG:
      *intermediate = argument8;
      *final        = mem_readw(*intermediate, 0);
      break;
*/
    case ADDR_ZPG:
      *final = argument8;
      break;

    case ADDR_ZPGX:
      *final = argument8+regs.x;
      break;

    case ADDR_ZPGY:
      *final = argument8+regs.y;
      break;

  }
  if ((*final >= 0) &&
      ((!strcmp(instruction[mem_readb(regs.pc, 0)].mnemonic,"JMP")) ||
       (!strcmp(instruction[mem_readb(regs.pc, 0)].mnemonic,"JSR"))))
    *final = -1;
}

//===========================================================================
BOOL InternalSingleStep () {
  BOOL result = 0;
  _try {
    ++profiledata[mem_readb(regs.pc, 0)];
    CpuExecute(stepline);
    result = 1;
  }
  _except (EXCEPTION_EXECUTE_HANDLER) {
    result = 0;
  }
  return result;
}

//===========================================================================
void OutputTraceLine () {
  char disassembly[50];
  char flags[9];

  DrawDisassembly((HDC)0,0,regs.pc,disassembly);
  DrawFlags((HDC)0,0,regs.ps,flags);
  fprintf(tracefile,
            "a=%02x x=%02x y=%02x sp=%03x ps=%s   %s\n",
            (unsigned)regs.a,
            (unsigned)regs.x,
            (unsigned)regs.y,
            (unsigned)regs.sp,
            (LPCTSTR)flags,
            (LPCTSTR)disassembly);
}

//===========================================================================
int ParseCommandString () {
  int args = 0;
  int loop;
  LPTSTR currptr = commandstring[0];

  while (*currptr) {
    LPTSTR   endptr = NULL;
    BOOL more;
    unsigned length = strlen(currptr);

    strtok(currptr," ,-=");
    strncpy(arg[args].str,currptr,11);
    arg[args].str[11] = 0;
    arg[args].val1    = (WORD)(strtoul(currptr,&endptr,16) & 0xFFFF);
    if (endptr)
      if (*endptr == 'L') {
        arg[args].val2 = (WORD)(strtoul(endptr+1,&endptr,16) & 0xFFFF);
        if (endptr && *endptr)
          arg[args].val2 = 0;
      }
      else {
        arg[args].val2 = 0;
        if (*endptr)
          arg[args].val1 = 0;
      }
    else
      arg[args].val2 = 0;
    more = ((*currptr) && (length > strlen(currptr)));
    args    += more;
    currptr += strlen(currptr)+more;
  }
  loop = args;
  while (loop++ < MAXARGS-1) {
    arg[loop].str[0] = 0;
    arg[loop].val1   = 0;
    arg[loop].val2   = 0;
  }
  return args;
}

//===========================================================================
void WriteProfileData () {
  char filename[MAX_PATH];
  FILE *file;

  strcpy(filename,progdir);
  strcat(filename,"Profile.txt");
  file = fopen(filename,"wt");
  if (file) {
    DWORD maxvalue;
    do {
      DWORD maxitem;
      DWORD loop;

      maxvalue = 0;
      for (loop = 0; loop < 256; ++loop)
        if (profiledata[loop] > maxvalue) {
          maxvalue = profiledata[loop];
          maxitem  = loop;
        }
      if (maxvalue) {
        fprintf(file,
                "%9u  %02X  %s\n",
                (unsigned)maxvalue,
                (unsigned)maxitem,
                (LPCTSTR)instruction[maxitem].mnemonic);
        profiledata[maxitem] = 0;
      }
    } while (maxvalue);
    fclose(file);
  }
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void DebugBegin ()
{
	mode = MODE_DEBUG;
	FrameRefreshStatus(DRAW_TITLE);
	addressmode[INVALID2].bytes = 2;
	addressmode[INVALID3].bytes = 3;
	ComputeTopOffset(regs.pc);
	DebugDisplay(1);
}

//===========================================================================
void DebugContinueStepping () {
  static unsigned stepstaken = 0;
  if (stepcount) {
    if (tracefile)
      OutputTraceLine();
    lastpc = regs.pc;
    InternalSingleStep();
    if ((regs.pc == stepuntil) || CheckBreakpoint(regs.pc,1))
      stepcount = 0;
    else if (stepcount > 0)
      stepcount--;
  }
  if (stepcount) {
    if (!((++stepstaken) & 0xFFFF))
      if (stepstaken == 0x10000)
        VideoRedrawScreen();
      else
        VideoRefreshScreen();
  }
  else {
    mode = MODE_DEBUG;
    FrameRefreshStatus(DRAW_TITLE);
    if ((stepstart < regs.pc) && (stepstart+3 >= regs.pc))
      topoffset += addressmode[instruction[mem_readb(topoffset, 0)].addrmode].bytes;
    else
      ComputeTopOffset(regs.pc);
    DebugDisplay(stepstaken >= 0x10000);
    stepstaken = 0;
  }
}

//===========================================================================
void DebugDestroy () {
  DebugEnd();
  DeleteObject(debugfont);
  FreeSymbolTable();
}

//===========================================================================
void DebugDisplay (BOOL drawbackground) {
  HDC dc = (HDC)FrameGetDC();
  SelectObject(dc,debugfont);
  SetTextAlign(dc,TA_TOP | TA_LEFT);

  // DRAW THE BACKGROUND
  if (drawbackground) {
    RECT viewportrect;
    viewportrect.left   = 0;
    viewportrect.top    = 0;
    viewportrect.right  = SCREENSPLIT1-14;
    viewportrect.bottom = 304;
    SetBkColor(dc,color[colorscheme][COLOR_INSTBKG]);
    ExtTextOut(dc,0,0,ETO_OPAQUE,&viewportrect,"",0,NULL);
    viewportrect.left   = SCREENSPLIT1-14;
    viewportrect.right  = 560;
    SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
    ExtTextOut(dc,0,0,ETO_OPAQUE,&viewportrect,"",0,NULL);
  }

  // DRAW DISASSEMBLED LINES
  {
    int  line   = 0;
    WORD offset = topoffset;
    while (line < SOURCELINES) {
      offset += DrawDisassembly(dc,line,offset,NULL);
      line++;
    }
  }

  // DRAW THE DATA AREA
  DrawStack(dc,0);
  DrawTargets(dc,10);
  DrawRegister(dc,13,"A" ,1,regs.a);
  DrawRegister(dc,14,"X" ,1,regs.x);
  DrawRegister(dc,15,"Y" ,1,regs.y);
  DrawRegister(dc,16,"PC",2,regs.pc);
  DrawRegister(dc,17,"SP",2,regs.sp);
  DrawFlags(dc,18,regs.ps,NULL);
  if (usingbp)
    DrawBreakpoints(dc,0);
  if (usingwatches)
    DrawWatches(dc,7);
  if (usingmemdump)
    DrawMemory(dc,14);

  // DRAW THE COMMAND LINE
  {
    int line = COMMANDLINES;
    while (line--)
      DrawCommandLine(dc,line);
  }
  // Draw Cumulative Cycles
  {
		RECT linerect;
		char valuestr[16];

		linerect.left   = SCREENSPLIT2;
		linerect.right  = SCREENSPLIT2 + 24;
		linerect.top    = 13 << 4;
		linerect.bottom = linerect.top + 16;
		SetTextColor(dc,color[colorscheme][COLOR_STATIC]);
		SetBkColor(dc,color[colorscheme][COLOR_DATABKG]);
		ExtTextOut(dc, linerect.left, linerect.top, ETO_CLIPPED | ETO_OPAQUE, &linerect,
			"Cyc", 3, NULL);
		wsprintf(valuestr, "%ld", g_dCumulativeCycles);
		linerect.left  = SCREENSPLIT2 + 28;
		linerect.right = SCREENSPLIT2 + 150;
		SetTextColor(dc,color[colorscheme][COLOR_DATATEXT]);
		ExtTextOut(dc, linerect.left, linerect.top, ETO_CLIPPED | ETO_OPAQUE, &linerect,
				 valuestr, strlen(valuestr), NULL);
  }
  FrameReleaseDC();
}

//===========================================================================
void DebugEnd () {
  if (profiling)
    WriteProfileData();
  if (tracefile) {
    fclose(tracefile);
    tracefile = NULL;
  }
}

//===========================================================================
void DebugInitialize () {

  // CLEAR THE BREAKPOINT AND WATCH TABLES
  ZeroMemory(breakpoint,BREAKPOINTS*sizeof(bprec));
  {
    int loop = 0;
    while (loop < WATCHES)
      watch[loop++] = -1;
  }

  // READ IN THE SYMBOL TABLE
  {
    char filename[MAX_PATH];
    int   symbolalloc = 0;
    WORD  lastvalue   = 0;
    FILE *infile;

    strcpy(filename,progdir);
    strcat(filename,NOMEARQSYM);
    infile = fopen(filename,"rt");
	if (!infile) {
      MessageBox(GetDesktopWindow(),
	             "Arquivo de Símbolo " NOMEARQSYM " "
	 			 "não achado.",
				 TITULO,
				 MB_ICONEXCLAMATION | MB_SETFOREGROUND);
	}
    if (infile) {
      while (!feof(infile)) {

        // READ IN THE NEXT LINE, AND MAKE SURE IT IS SORTED CORRECTLY IN
        // VALUE ORDER
        DWORD value     = 0;
        char  name[14]  = "";
        char  line[256];
        fscanf(infile,"%x %13s",&value,name);
        fgets(line,255,infile);
        if (value)
          if (value < lastvalue) {
            MessageBox(GetDesktopWindow(),
                       "O arquivo de símbolos não está ordenado corretamente.\n"
                       "Os símbolos não serão carregados.",
                       TITULO,
                       MB_ICONEXCLAMATION | MB_SETFOREGROUND);
            FreeSymbolTable();
            return;
          }
          else {

            // IF OUR CURRENT SYMBOL TABLE IS NOT BIG ENOUGH TO HOLD THIS
            // ADDITIONAL SYMBOL, THEN ALLOCATE A BIGGER TABLE AND COPY THE
            // CURRENT DATA ACROSS
            if ((!symboltable) || (symbolalloc <= symbolnum)) {
              symbolptr newtable;

              symbolalloc += 8192/sizeof(symbolrec);
              newtable = (symbolptr)VirtualAlloc(NULL,
                                                           symbolalloc*sizeof(symbolrec),
                                                           MEM_COMMIT,
                                                           PAGE_READWRITE);
              if (newtable) {
                if (symboltable) {
                  CopyMemory(newtable,symboltable,symbolnum*sizeof(symbolrec));
                  VirtualFree(symboltable,0,MEM_RELEASE);
                }
                symboltable = newtable;
              }
              else {
                MessageBox(GetDesktopWindow(),
                           "Não há memória suficiente para carregar "
                           "o arquivo de símbolos.",
                           TITULO,
                           MB_ICONEXCLAMATION | MB_SETFOREGROUND);
                FreeSymbolTable();
              }
            }
            // SAVE THE NEW SYMBOL IN THE SYMBOL TABLE
            if (symboltable) {
              symboltable[symbolnum].value = (WORD)(value & 0xFFFF);
#ifdef UNICODE
              mbstowcs(symboltable[symbolnum].name,name,12);
#else
              strncpy(symboltable[symbolnum].name,name,12);
#endif
              symboltable[symbolnum].name[13] = 0;
              symbolnum++;
            }

            lastvalue = (WORD)value;
          }

      }
      fclose(infile);
    }
  }

  // CREATE A FONT FOR THE DEBUGGING SCREEN
  debugfont = CreateFont(15,0,0,0,FW_MEDIUM,0,0,0,OEM_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,FIXED_PITCH | 4 | FF_MODERN,
                         "Courier New");

}

//===========================================================================
void DebugProcessChar (char ch)
{
	if ((mode == MODE_STEPPING) && (ch == '\x1B'))
		stepcount = 0;
	if (mode != MODE_DEBUG)
		return;
	if ((ch == ' ') && !commandstring[0][0])
		return;
	if ((ch >= 32) && (ch <= 126))
	{
		int length = strlen(commandstring[0]);
		HDC dc = (HDC)FrameGetDC();

		ch = (char)CharUpper((LPTSTR)ch);
		if (length < 68)
		{
			commandstring[0][length]   = ch;
			commandstring[0][length+1] = 0;
		}
		DrawCommandLine(dc,0);
		FrameReleaseDC();
	}
}

//===========================================================================
void DebugProcessCommand (int keycode) {
  BOOL needscmdrefresh  = 0;
  BOOL needsfullrefresh = 0;

  if (mode != MODE_DEBUG)
    return;
  if (viewingoutput) {
    DebugDisplay(1);
    viewingoutput = 0;
    return;
  }
  if ((keycode == VK_SPACE) && commandstring[0][0])
    return;
  if (keycode == VK_BACK) {
    int length = strlen(commandstring[0]);
    if (length)
      commandstring[0][length-1] = 0;
    needscmdrefresh = 1;
  }
  else if (keycode == VK_RETURN) {
    int loop;

    if ((!commandstring[0][0]) &&
        (commandstring[1][0] != ' '))
      strcpy(commandstring[0],commandstring[1]);
    loop = COMMANDLINES-1;
    while (loop--)
      strcpy(commandstring[loop+1],commandstring[loop]);
    needscmdrefresh  = COMMANDLINES;
    needsfullrefresh = ExecuteCommand(ParseCommandString());
    commandstring[0][0] = 0;
  }
  else switch (keycode) {
    case VK_SPACE:  needsfullrefresh = CmdTrace(0);     break;
	case VK_F10:    needsfullrefresh = CmdTraceOut(0);  break;
    case VK_PRIOR:  needsfullrefresh = CmdPageUp(0);    break;
    case VK_NEXT:   needsfullrefresh = CmdPageDown(0);  break;
    case VK_UP:     needsfullrefresh = CmdLineUp(0);    break;
    case VK_DOWN:   needsfullrefresh = CmdLineDown(0);  break;
  }
  if (needsfullrefresh)
    DebugDisplay(0);
  else if (needscmdrefresh && (!viewingoutput) &&
           ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))) {
    HDC dc = (HDC)FrameGetDC();
    while (needscmdrefresh--)
      DrawCommandLine(dc,needscmdrefresh);
    FrameReleaseDC();
  }
}

// EOF