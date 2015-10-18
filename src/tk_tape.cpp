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

// Emula um tape (ainda não implementado)

#include "tk_stdhdr.h"
#include "tk_tape.h"
#include "tk_main.h"
#include "tk_memoria.h"
#include "tk_registro.h"
#include "tk_janela.h"

// Definições

// Variáveis
static FILE *HandleArq = NULL;
static char NomeTitulo[MAX_PATH];
static char *Buffer;
static int  PosBuffer = 0;
static int  TamBuffer = 0;
static int  TamDados  = 0;

// Protótipos

// Funções Internas
//===========================================================================
void TapePegaTitulo(char *NomeImagem) {
	BOOL found = 0;
	int  loop  = 0;

	char  imagetitle[128];
	char* startpos = NomeImagem;
	if (strrchr(startpos,'\\'))
		startpos = strrchr(startpos,'\\')+1;
	strncpy(imagetitle, startpos, 127);
	imagetitle[127] = 0;
	while (imagetitle[loop] && !found)
		if (IsCharLower(imagetitle[loop]))
			found = 1;
		else
			loop++;
	if ((!found) && (loop > 2))
		CharLowerBuff(imagetitle+1, strlen(imagetitle+1));
	strncpy(NomeTitulo, imagetitle, 127);
	NomeTitulo[127] = 0;
	if (imagetitle[0]) {
		char *dot = imagetitle;
		if (strrchr(dot,'.'))
			dot = strrchr(dot,'.');
		if (dot > imagetitle)
			*dot = 0;
	}
	strncpy(NomeTitulo, imagetitle, 30);
	NomeTitulo[30] = 0;
}

// Funções Globais
//===========================================================================
void TapeInicializa() {
	//
}

//===========================================================================
void TapeAtualiza(DWORD totalcycles) {
	//FrameRefreshStatus(DRAW_LEDS);
}

//===========================================================================
void TapeFinaliza() {
	TapeRemove();
}

#ifdef _WINDOWS
//===========================================================================
void TapeSelect() {
	char directory[MAX_PATH] = "";
	char filename[MAX_PATH]  = "";
	char title[40];
	OPENFILENAME ofn;

	RegLoadString(PREFERENCIAS, DIRINIC, 1, directory, MAX_PATH);
	strcpy(title,"Selecione a imagem do tape");
	ZeroMemory(&ofn,sizeof(OPENFILENAME));
	ofn.lStructSize     = sizeof(OPENFILENAME);
	ofn.hwndOwner       = framewindow;
	ofn.hInstance       = instance;
	ofn.lpstrFilter     = "Imagens de Tape (*.ct2)\0*.ct2\0";
	ofn.lpstrFile       = filename;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = directory;
	ofn.Flags           = OFN_PATHMUSTEXIST;
	ofn.lpstrTitle      = title;
	if (GetOpenFileName(&ofn)) {
		int error;

		if ((!ofn.nFileExtension) || !filename[ofn.nFileExtension])
			strcat(filename,".ct2");
		error = TapeInsere(filename, ofn.Flags & OFN_READONLY, 1);
		if (!error) {
			//strcpy(NomeTitulo, filename + ofn.nFileOffset);
			TapePegaTitulo(filename + ofn.nFileOffset);
			filename[ofn.nFileOffset] = 0;
			if (stricmp(directory, filename))
				RegSaveString(PREFERENCIAS, DIRINIC, 1, filename);
			FrameRefreshStatus(DRAW_BACKGROUND | DRAW_LEDS);
		}
		else
			TapeNotifyInvalidImage(filename, error);
	}
}
#else // _WINDOWS
//===========================================================================
void TapeSelect() {
	// Implementar no linux
}
#endif // _WINDOWS

//===========================================================================
int TapeInsere(char *imagefilename, int writeprotected, int createifnecessary) {
	TCh    Ch;

	if (HandleArq)
		TapeRemove();

	HandleArq = fopen(imagefilename, "rb");
	if (!HandleArq)
		return 1; // retorna erro

	fseek(HandleArq, 0, SEEK_END);
	TamBuffer = ftell(HandleArq);
	fseek(HandleArq, 0, SEEK_SET);
	if (!(Buffer = (char*)malloc(TamBuffer+1)))
		return 1;
	fread(Buffer, 1, TamBuffer, HandleArq);
	fclose(HandleArq);
	memcpy(&Ch, Buffer, 4);
	if (strncmp((char*)&Ch, CT2_MAGIC, 4)) {
		free(Buffer);
		TamBuffer = 0;
		return 2; // retorna erro
	}
	PosBuffer = 0;
	return 0; // sem erro
}

//===========================================================================
void TapeRemove() {
	if (TamBuffer) {
		free(Buffer);
		TamBuffer = 0;
	}
}

//===========================================================================
void TapeBotao(int Botao) {
	switch(Botao) {
		case TB_REW:
			PosBuffer = 0;
		break;

		case TB_STOP:
			PosBuffer = TamBuffer;
		break;
	}
	FrameRefreshStatus(DRAW_LEDS);
}

//===========================================================================
void TapeNotifyInvalidImage (char* imagefilename,int error)
{
	char buffer[400];
	switch (error) {

		case 1:
			sprintf(buffer, "Erro ao abrir o arquivo %s.", imagefilename);
			FrameMostraMensagemAdvertencia(buffer);
			break;

		case 2:
			sprintf(buffer,
					"Erro ao abrir o arquivo %s.\nO formato "
					"da imagem do disco não foi reconhecido.",
					imagefilename);
			FrameMostraMensagemAdvertencia(buffer);
			break;

		default:
			// IGNORE OTHER ERRORS SILENTLY
			return;
	}
}

//===========================================================================
char *TapeNomeImagem() {
	return NomeTitulo;
}

//===========================================================================
void TapePegaStatusLed(int *Led) {
	*Led = (PosBuffer > 0 && PosBuffer < TamBuffer);
}

// Softswitches
//===========================================================================
BYTE __stdcall TapeMOTOR(WORD programcounter, BYTE address, BYTE write, BYTE value) {
	return MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall TapeCASOUT(WORD programcounter, BYTE address, BYTE write, BYTE value) {
	return MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall TapeCASIN(WORD programcounter, BYTE address, BYTE write, BYTE value) {
	TCh    Ch;

	while (PosBuffer < TamBuffer) {
		if (TamDados) {
			char c = Buffer[PosBuffer++];

			TamDados--;
			FrameRefreshStatus(DRAW_LEDS);
			return c;
		} else {
			memcpy(&Ch, (Buffer + PosBuffer++), 4);
			if (!strncmp((char *)&Ch.ID, CT2_DADOS, 2)) {
				PosBuffer += 3;
				TamDados = Ch.Tam;
			}
		}
	}
	FrameRefreshStatus(DRAW_LEDS);
	return MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall TapeCASIN2(WORD programcounter, BYTE address, BYTE write, BYTE value) {
	TCh    Ch;

	while (PosBuffer < TamBuffer) {
		memcpy(&Ch, (Buffer + PosBuffer++), 4);
		if (!strncmp((char *)Ch.ID, CT2_CAB_B, 2)) {
			TamDados = 0;
			PosBuffer += 3;
			return 1;
		}
	}
	FrameRefreshStatus(DRAW_LEDS);
	return 0;
}


// EOF