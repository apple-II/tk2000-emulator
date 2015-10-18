/****************************************************************************
*
*  APPLE //E EMULATOR FOR WINDOWS                    
*
*  Copyright (C) 1994-96, Michael O'Brien.  All rights reserved.
*
***/

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

// Utilitários gerais

#include "tk_stdhdr.h"
#include "tk_memoria.h"

/* Constantes */
const char applesoft_tokens[][25] = {
/* 80 */ "END", "FOR", "NEXT", "DATA", "INPUT", "DEL", "DIM", "READ", 
/* 88 */ "GR", "TEXT", "DSK", "ASS", "CALL", "PLOT", "HLIN", "VLIN", 
/* 90 */ "HGR2", "HGR", "HCOLOR=", "HPLOT", "DRAW", "XDRAW", "HTAB", "HOME", 
/* 98 */ "ROT=", "SCALE=", "SHLOAD", "TRACE", "NOTRACE", "NORMAL", "INVERSE", "SOUND", 
/* A0 */ "COLOR", "POP", "VTAB ", "HIMEM:", "LOMEM:", "ONERR", "RESUME", "RECALL", 
/* A8 */ "STORE", "SPEED=", "LET", "GOTO", "RUN", "IF", "RESTORE", "&", 
/* B0 */ "GOSUB", "RETURN", "REM", "STOP", "ON", "WAIT", "LOAD", "SAVE", 
/* B8 */ "DEF", "POKE", "PRINT", "CONT", "LIST", "CLEAR", "GET", "NEW", 
/* C0 */ "TAB (", "TO", "FN", "SPC(", "THEN", "AT", "NOT", "STEP", 
/* C8 */ "+", "-", "*", "/", "^", "AND", "OR", ">", 
/* D0 */ "=", "<", "SGN", "INT", "ABS", "USR", "FRE", "SCRN (", 
/* D8 */ "PDL", "POS", "SQR", "RND", "LOG", "EXP", "COS", "SIN", 
/* E0 */ "TAN", "ATN", "PEEK", "LEN", "STR$", "VAL", "ASC", "CHR$", 
/* E8 */ "LEFT$", "RIGHT$", "MID$", "LM", "MOTOR", "TK2000", "MP", "MA", 
/* F0 */ " SINTAXE ", " RETURN SEM GOSUB ", " NAO HA MAIS DATA ", " VALOR ILEGAL ", " ESTOURO ", " EXCEDE MEMORIA ", " COMANDO NAO DEFINIDO ", " INDICE ILEGAL ", 
/* F8 */ " RE'DIM' DE ARRAY ", " DIVISAO POR ZERO ", " COMANDO ILEGAL ", " INCOPATIVEL ", " EXCEDE STRING ", " FORMULA MUITO COMPLEXA ", " IMPOSSIVEL ", ""
};

/* Variáveis internas */
BYTE* page;
WORD  addr;
WORD initAddress = 0x0801;		// Applesoft initial address
WORD nextAddress;
WORD lineNumber;
char *buffer = NULL;
unsigned int len = 0, size = 1024*1024; // 1MB


/* funções internas */

/* Acesso direto à memória */
BYTE dreadb() {
	BYTE  result = 0;

	page = memread[addr >> 8];
	if (page)
		result = *(page+(addr & 0xFF));

	addr++;
	return result;
}

WORD dreadw() {
	WORD  result = 0;

	page = memread[addr >> 8];
	if (page)
		result = *(BYTE*)(page+(addr & 0xFF));
	else
		result = 0;
	addr++;
	page = memread[addr >> 8];
	if (page)
		result |= (*(BYTE*)(page+(addr & 0xFF))) << 8;
	else
		result |= 0;
	addr++;
	return result;
}

void addBuffer(char *text) {
	len += strlen(text);
	if (len > size) {
		size *= 2;
		buffer = (char *)realloc(buffer, size);
	}
	strcat(buffer, text);
}

/* funções globais */
char *UtilExtrairApplesoft() {
	char line[1024];
	unsigned char ch;

	addr = 0x67;
	initAddress = dreadw();
	if (initAddress < 0x800 || initAddress > 0xBFFF) {
		return NULL;
	}
	buffer = (char *)malloc(size);
	buffer[0] = 0;

	addr = initAddress;
	nextAddress = dreadw();
	while (nextAddress > 0x7FF && nextAddress < 0xC000) {
		lineNumber = dreadw();
		sprintf(line, "%d ", lineNumber);
		addBuffer(line);
		while ( (ch = dreadb()) != 0 ) {
			if (ch >= 0x80) {
				sprintf(line, " %s ", applesoft_tokens[ch-0x80]);
				addBuffer(line);
			} else {
				sprintf(line, "%c", ch);
				addBuffer(line);
			}
		}
		addBuffer("\n");
		addr = nextAddress;
		nextAddress = dreadw();
	}

	return buffer;
}
