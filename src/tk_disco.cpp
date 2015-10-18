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

// Emula o DISK II

#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_disco.h"
#include "tk_janela.h"
#include "tk_imagem.h"
#include "tk_memoria.h"
#include "tk_registro.h"

#define  DRIVE_1  0
#define  DRIVE_2  1

#define  DRIVES   2
#define  TRACKS   35

typedef struct _floppyrec {
	char   imagename[16];
	char   fullname[128];
	HIMAGE imagehandle;
	BYTE   Volume;
	int    track;
	BYTE*  trackimage;
	int    phase;
	int    byte;
	BOOL   writeprotected;
	BOOL   trackimagedata;
	BOOL   trackimagedirty;
	DWORD  spinning;
	DWORD  writelight;
	int    nibbles;
} floppyrec, *floppyptr;

// Variáveis
int			currdrive       = 0;
BOOL		diskaccessed    = 0;
BOOL		enhancedisk     = 1;
floppyrec	floppy[DRIVES];
BYTE		floppylatch     = 0;
BOOL		floppymotoron   = 0;
BOOL		floppywritemode = 0;
BYTE		SlotInserido    = 0;

void ReadTrack (int drive);
void WriteTrack (int drive);

//===========================================================================
void CheckSpinning() {
	DWORD modechange = (floppymotoron && !floppy[currdrive].spinning);
	if (floppymotoron)
		floppy[currdrive].spinning = 20000;
	if (modechange)
		FrameRefreshStatus(DRAW_LEDS);
}

//===========================================================================
void GetImageTitle (char* imagefilename, floppyptr fptr) {
	BOOL found = 0;
	int  loop  = 0;

	char   imagetitle[128];
	char* startpos = imagefilename;
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
		CharLowerBuff(imagetitle+1,strlen(imagetitle+1));
	strncpy(fptr->fullname,imagetitle,127);
	fptr->imagename[127] = 0;
	if (imagetitle[0]) {
		char *dot = imagetitle;
		if (strrchr(dot,'.'))
			dot = strrchr(dot,'.');
		if (dot > imagetitle)
			*dot = 0;
	}
	strncpy(fptr->imagename,imagetitle,30);
	fptr->imagename[30] = 0;
}

//===========================================================================
void ReadTrack (int drive) {
	floppyptr fptr = &floppy[drive];
	if (fptr->track >= TRACKS) {
		fptr->trackimagedata = 0;
		return;
	}
	if (!fptr->trackimage)
		fptr->trackimage = (BYTE*)malloc(0x1A00);
		//fptr->trackimage = (BYTE*)VirtualAlloc(NULL,0x1A00,MEM_COMMIT,PAGE_READWRITE);
	if (fptr->trackimage && fptr->imagehandle) {
		ImageReadTrack(fptr->imagehandle,
						fptr->track,
						fptr->phase,
						fptr->trackimage,
						&fptr->nibbles);
		fptr->byte           = 0;
		fptr->trackimagedata = (fptr->nibbles != 0);
	}
}

//===========================================================================
void WriteTrack (int drive) {
	floppyptr fptr = &floppy[drive];

	if (fptr->track >= TRACKS)
		return;
	if (fptr->trackimage && fptr->imagehandle)
		ImageWriteTrack(fptr->imagehandle,
						fptr->track,
						fptr->phase,
						fptr->trackimage,
						fptr->nibbles);
	fptr->trackimagedirty = 0;
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
int DiscoConfiguraSlotAux() {
	iofunction ioarrayread[0x10];
	iofunction ioarraywrite[0x10];

	ioarrayread[0x00] = DiskControlStepper;
	ioarrayread[0x01] = DiskControlStepper;
	ioarrayread[0x02] = DiskControlStepper;
	ioarrayread[0x03] = DiskControlStepper;
	ioarrayread[0x04] = DiskControlStepper;
	ioarrayread[0x05] = DiskControlStepper;
	ioarrayread[0x06] = DiskControlStepper;
	ioarrayread[0x07] = DiskControlStepper;
	ioarrayread[0x08] = DiskControlMotor;
	ioarrayread[0x09] = DiskControlMotor;
	ioarrayread[0x0A] = DiskEnable;
	ioarrayread[0x0B] = DiskEnable;
	ioarrayread[0x0C] = DiskReadWrite;
	ioarrayread[0x0D] = DiskSetLatchValue;
	ioarrayread[0x0E] = DiskSetReadMode;
	ioarrayread[0x0F] = DiskSetWriteMode;

	ioarraywrite[0x00] = DiskControlStepper;
	ioarraywrite[0x01] = DiskControlStepper;
	ioarraywrite[0x02] = DiskControlStepper;
	ioarraywrite[0x03] = DiskControlStepper;
	ioarraywrite[0x04] = DiskControlStepper;
	ioarraywrite[0x05] = DiskControlStepper;
	ioarraywrite[0x06] = DiskControlStepper;
	ioarraywrite[0x07] = DiskControlStepper;
	ioarraywrite[0x08] = DiskControlMotor;
	ioarraywrite[0x09] = DiskControlMotor;
	ioarraywrite[0x0A] = DiskEnable;
	ioarraywrite[0x0B] = DiskEnable;
	ioarraywrite[0x0C] = DiskReadWrite;
	ioarraywrite[0x0D] = DiskSetLatchValue;
	ioarraywrite[0x0E] = DiskSetReadMode;
	ioarraywrite[0x0F] = DiskSetWriteMode;

	if (MemInsereSlotAux(ioarrayread, ioarraywrite, "Disco")) {
		// Não teve erro;
		SlotInserido = 1;
		FrameMenuDiskete(1);
		return 1;
	}
	return 0; // Ocorreu um erro
}

//===========================================================================
void DiscoRetiraSlotAux() {
	if (SlotInserido)
		DiskDestroy();
	SlotInserido = 0;
	FrameMenuDiskete(0);
}

//===========================================================================
void DiskBoot() {
	// THIS FUNCTION RELOADS A PROGRAM IMAGE IF ONE IS LOADED IN DRIVE ONE.
	// IF A DISK IMAGE OR NO IMAGE IS LOADED IN DRIVE ONE, IT DOES NOTHING.
	if (floppy[0].imagehandle && ImageBoot(floppy[0].imagehandle))
		floppymotoron = 0;
}

//===========================================================================
void DiskDestroy() {
	DiskRemove(0);
	DiskRemove(1);
}

//===========================================================================
char *DiskGetFullName(int drive) {
	return floppy[drive].fullname;
}

//===========================================================================
void DiskGetLightStatus (int *drive1, int *drive2) {
	*drive1 = floppy[0].spinning ? floppy[0].writelight ? 2
														: 1
														: 0;
	*drive2 = floppy[1].spinning ? floppy[1].writelight ? 2
														: 1
														: 0;
}

//===========================================================================
char *DiskGetName (int drive) {
	return floppy[drive].imagename;
}

//===========================================================================
void DiskInitialize() {
	int loop = DRIVES;

	while (loop--)
		memset(&floppy[loop], 0, sizeof(floppyrec));
}

//===========================================================================
int DiskInsert (int drive, char *imagefilename, int writeprotected, int createifnecessary) {
	floppyptr fptr = &floppy[drive];
	int error;

	if (!SlotInserido)
		return -1;

	if (fptr->imagehandle)
		DiskRemove(drive);
	memset(fptr, 0, sizeof(floppyrec));
	//ZeroMemory(fptr,sizeof(floppyrec));
	fptr->writeprotected = writeprotected;
	error = ImageOpen(imagefilename,
							&fptr->Volume,
							&fptr->imagehandle,
							&fptr->writeprotected,
							createifnecessary);
	if (!error) {
		fptr->Volume = VOLUME;
		((imageinfoptr)(fptr->imagehandle))->Volume = fptr->Volume;
		GetImageTitle(imagefilename,fptr);
	}
	return error;
}

//===========================================================================
void DiskRemove(int drive) {
	floppyptr fptr = &floppy[drive];
	if (fptr->imagehandle) {
		if (fptr->trackimage && fptr->trackimagedirty)
			WriteTrack(drive);
		ImageClose(fptr->imagehandle);
		fptr->imagehandle = (HIMAGE)0;
	}
	if (fptr->trackimage) {
		//VirtualFree(fptr->trackimage,0,MEM_RELEASE);
		free(fptr->trackimage);
		fptr->trackimage     = NULL;
		fptr->trackimagedata = 0;
	}
	fptr->imagename[0] = '\0';

}

//===========================================================================
void DiskEscolheVolume(int drive) {
	floppyptr fptr = &floppy[drive];

	if (fptr->imagehandle) {
		fptr->Volume = (BYTE)FramePerguntaInteiro("Digite Volume do Disco", VOLUME);
		((imageinfoptr)(fptr->imagehandle))->Volume = fptr->Volume;
	}
}

//===========================================================================
BOOL DiskIsSpinning() {
	return floppymotoron;
}

//===========================================================================
void DiskNotifyInvalidImage (char* imagefilename,int error) {
	char buffer[400];
	switch (error) {

		case 1:
			sprintf(buffer,
					"Erro ao abrir o arquivo %s.",
					(char*)imagefilename);
			FrameMostraMensagemAdvertencia(buffer);
		break;

		case 2:
			sprintf(buffer,
					"Erro ao abrir o arquivo %s.\nO formato "
					"da imagem do disco não foi reconhecido.",
					(char*)imagefilename);
			FrameMostraMensagemAdvertencia(buffer);
		break;

		default:
			// IGNORE OTHER ERRORS SILENTLY
		return;
	}
}

//===========================================================================
void DiskReset() {
	floppymotoron = 0;
}

#ifdef _WINDOWS
//===========================================================================
void DiskSelect(int drive) {
	char directory[MAX_PATH] = "";
	char filename[MAX_PATH]  = "";
	char title[40];
	OPENFILENAME ofn;

	RegLoadString(PREFERENCIAS, DIRINIC, 1, directory, MAX_PATH);
	strcpy(title,"Selecione a imagem do disco ");
	strcat(title,drive ? "2" : "1");
	ZeroMemory(&ofn,sizeof(OPENFILENAME));
	ofn.lStructSize     = sizeof(OPENFILENAME);
	ofn.hwndOwner       = (HWND)framewindow;
	ofn.hInstance       = (HINSTANCE)instance;
	ofn.lpstrFilter     = "Imagens de Disco \0*.apl;*.do;*.dsk;*.iie;*.nib;*.po\0"
							"Imagens zipadas \0*.zip\0"
							"Todos os Arquivos\0*.*\0";
	ofn.lpstrFile       = filename;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrInitialDir = directory;
	ofn.Flags           = OFN_PATHMUSTEXIST;
	ofn.lpstrTitle      = title;
	if (GetOpenFileName(&ofn))
	{
		int error;

		if ((!ofn.nFileExtension) || !filename[ofn.nFileExtension])
			strcat(filename,".dsk");
		error = DiskInsert(drive,filename,ofn.Flags & OFN_READONLY,1);
		if (!error)
		{
			filename[ofn.nFileOffset] = 0;
			if (stricmp(directory,filename))
				RegSaveString(PREFERENCIAS, DIRINIC, 1, filename);
		}
		else
			DiskNotifyInvalidImage(filename,error);
	}
}
#else // _WINDOWS
//===========================================================================
void DiskSelect(int drive) {

}
#endif // _WINDOWS

//===========================================================================
void DiskUpdatePosition(DWORD cycles) {
	int loop = 2;
	while (loop--) {
		floppyptr fptr = &floppy[loop];
		if (fptr->spinning && !floppymotoron) {
			if (!(fptr->spinning -= MIN(fptr->spinning,(cycles >> 6))))
			FrameRefreshStatus(DRAW_LEDS);
		}
		if (floppywritemode && (currdrive == loop) && fptr->spinning)
			fptr->writelight = 20000;
		else if (fptr->writelight) {
			if (!(fptr->writelight -= MIN(fptr->writelight,(cycles >> 6))))
				FrameRefreshStatus(DRAW_LEDS);
		}
		if ((!enhancedisk) && (!diskaccessed) && fptr->spinning) {
			fptr->byte += (cycles >> 5);
			if (fptr->byte >= fptr->nibbles)
				fptr->byte -= fptr->nibbles;
		}
	}
	diskaccessed = 0;
}


// Soft-switches
//===========================================================================
BYTE __stdcall DiskReadWrite (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (!floppymotoron)
		return 0;
	floppyptr fptr = &floppy[currdrive];
	BYTE result;

	diskaccessed = 1;
	if ((!fptr->trackimagedata) && fptr->imagehandle)
		ReadTrack(currdrive);
	if (!fptr->trackimagedata)
		return 0xFF;
	result = 0;
	if ((!floppywritemode) || (!fptr->writeprotected))
		if (floppywritemode)
			if (floppylatch & 0x80) {
				*(fptr->trackimage+fptr->byte) = floppylatch;
				fptr->trackimagedirty = 1;
			} else
				return 0;
		else
			result = *(fptr->trackimage+fptr->byte);
		if (++fptr->byte >= fptr->nibbles)
			fptr->byte = 0;
	return result;
}

//===========================================================================
BYTE __stdcall DiskSetLatchValue(WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (!floppymotoron)
		return 0;
	if (write)
		floppylatch = value;
	return floppylatch;
}

//===========================================================================
BYTE __stdcall DiskSetReadMode (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (!floppymotoron)
		return 0;
	floppywritemode = 0;
	return MemReturnRandomData(floppy[currdrive].writeprotected);
}

//===========================================================================
BYTE __stdcall DiskSetWriteMode (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (!floppymotoron)
		return 0;
	BOOL modechange;

	floppywritemode = 1;
	modechange = !floppy[currdrive].writelight;
	floppy[currdrive].writelight = 20000;
	if (modechange)
		FrameRefreshStatus(DRAW_LEDS);
	return MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall DiskControlMotor (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	floppymotoron = address & 1;
	CheckSpinning();
	return MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall DiskControlStepper (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	if (!floppymotoron)
		return 0;
	floppyptr fptr = &floppy[currdrive];
	int direction = 0;
	if (address & 1) {
		int phase     = (address >> 1) & 3;
		if (phase == ((fptr->phase+1) & 3))
			direction = 1;
		if (phase == ((fptr->phase+3) & 3))
			direction = -1;
		if (direction) {
			fptr->phase = MAX(0,MIN(79,fptr->phase+direction));
			if (!(fptr->phase & 1)) {
				int newtrack = MIN(TRACKS-1,fptr->phase >> 1);
				if (newtrack != fptr->track) {
					if (fptr->trackimage && fptr->trackimagedirty)
						WriteTrack(currdrive);
					fptr->track          = newtrack;
					fptr->trackimagedata = 0;
				}
			}
		}
	}

	return (address == 0xE0) ? 0xFF : MemReturnRandomData(1);
}

//===========================================================================
BYTE __stdcall DiskEnable (WORD programcounter, BYTE address, BYTE write, BYTE value) {
	currdrive = address & 1;
	floppy[!currdrive].spinning   = 0;
	floppy[!currdrive].writelight = 0;
	CheckSpinning();

	return 0;
}

// EOF