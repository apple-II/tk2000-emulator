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

// Módulo Principal

#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_memoria.h"
#include "tk_cpu.h"
#include "tk_joystick.h"
#include "tk_video.h"
#include "tk_som_nucleo.h"
#include "tk_som.h"
#include "tk_teclado.h"
#include "tk_debug.h"
#include "tk_registro.h"
#include "tk_janela.h"
#include "tk_tape.h"
#include "tk_timer.h"
#include "tk_impressora.h"
#include "tk_disco.h"
#include "tk_imagem.h"

// Definições:
#define REGISTRAREXTENSOES

// Variáveis:
#ifdef CPUDEBUG
FILE *arquivocpu;
#endif

HINSTANCE instance       = 0;
DWORD  cumulativecycles  = 0;
DWORD  cyclenum          = 0;
BOOL   fullspeed         = 0;
//DWORD  lastfastpaging    = 0;
//DWORD  lasttrimimages    = 0;
int    mode              = MODE_LOGO;
int    lastmode          = MODE_LOGO;
char   progdir[MAX_PATH] = "";
char   tempdir[MAX_PATH] = "";
BOOL   resettiming       = 0;
BOOL   restart           = 0;
DWORD  speed             = 10;
DWORD  dummy             = 0;
int	   numexpages        = 16;
int    RWatual           = 0;

int    g_nCpuCyclesFeedback = 0;
double g_fCurrentCLK6502 = CLK_6502;
unsigned __int64 g_nCumulativeCycles = 0;
double g_fMHz            = 1.0;	// Affected by Config dialog's speed slider bar
FILE*  g_fh              = NULL;

// Funções:
//===========================================================================
void ContinueExecution ()
{
	static   DWORD dwVideoUpdateTick = 0;
	static   BOOL pageflipping    = 0; //?
	int      nNumCyclesPerFrame = 23191;
	DWORD    nCyclesToExecute;
	DWORD    executedcycles;
	DWORD    CLKS_PER_MS;
	BOOL     diskspinning;
	BOOL     screenupdated;
	BOOL     systemidle;

	fullspeed = ( (speed == SPEED_MAX) || 
				 (GetKeyState(VK_SCROLL) < 0) ||
				 (ColarRapido && KeybIsPasting()) ||
				 (DiskIsSpinning() && enhancedisk));

	if(fullspeed)
	{
		Timer_StopTimer();
	}
	else
	{
		Timer_StartTimer((ULONG)((double)nNumCyclesPerFrame / g_fMHz));
	}

	nCyclesToExecute = nNumCyclesPerFrame + g_nCpuCyclesFeedback;
	executedcycles = CpuExecute(nCyclesToExecute);

	cyclenum = executedcycles;
	
	// Atualizações:
	KeybAtualiza(executedcycles);
	DiskUpdatePosition(executedcycles);
	JoyUpdatePosition(executedcycles);
//	VideoUpdateVbl(executedcycles, 0);  /* TO DO: nearrefresh */
//	HDAtualiza(executedcycles);
//	IDEAtualiza(executedcycles);
//	TKClockAtualiza(executedcycles);
	TapeAtualiza(executedcycles);
//	CommUpdate(executedcycles);
	ImpressoraAtualiza(executedcycles);
	SpkrUpdate(executedcycles);
	
	CLKS_PER_MS = (DWORD)g_fCurrentCLK6502 / 1000;

	emulmsec_frac += executedcycles;
	if(emulmsec_frac > CLKS_PER_MS)
	{
		emulmsec += emulmsec_frac / CLKS_PER_MS;
		emulmsec_frac %= CLKS_PER_MS;
	}

	//
	// DETERMINE WHETHER THE SCREEN WAS UPDATED, THE DISK WAS SPINNING,
	// OR THE KEYBOARD I/O PORTS WERE BEING EXCESSIVELY QUERIED THIS CLOCKTICK
	VideoCheckPage(0);
	diskspinning  = DiskIsSpinning();
	screenupdated = VideoHasRefreshed();
	systemidle    = 0;

	if(screenupdated)
		pageflipping = 3;

	//
	// IF A TWENTIETH OF A SECOND HAS ELAPSED AND THE SCREEN HAS NOT BEEN
	// UPDATED BUT IT APPEARS TO NEED UPDATING, THEN REFRESH IT
	if(mode != MODE_LOGO)
	{
		const DWORD _50ms = 50;	// 50ms == 20Hz
		DWORD dwCyclesPerScreenUpdate = _50ms * (DWORD) (g_fCurrentCLK6502 / 1000.0);

		if((GetTickCount() - dwVideoUpdateTick) > _50ms)
		{
			static BOOL  anyupdates     = 0;
			static DWORD lastcycles     = 0;
			static BOOL  lastupdates[2] = {0,0};

			dwVideoUpdateTick = GetTickCount();

			anyupdates |= screenupdated;

			if ((cumulativecycles-lastcycles) >= dwCyclesPerScreenUpdate)
			{
				lastcycles = cumulativecycles;
				if ((!anyupdates) && (!lastupdates[0]) && (!lastupdates[1]) &&
					VideoApparentlyDirty())
				{
					static DWORD lasttime = 0;
					DWORD currtime = GetTickCount();

					VideoCheckPage(1);
					if ((!fullspeed) ||
						(currtime-lasttime >= (DWORD)((graphicsmode || !systemidle) ? 100 : 25)))
					{
						VideoRefreshScreen();
						lasttime = currtime;
					}
					screenupdated = 1;
				}

				lastupdates[1] = lastupdates[0];
				lastupdates[0] = anyupdates;
				anyupdates     = 0;

				if (pageflipping)
					pageflipping--;
			}
		}
	}
	if(!fullspeed)
	{
		Timer_WaitTimer();
	}
}

//===========================================================================
void SetCurrentCLK6502()
{
	static DWORD dwPrevSpeed = (DWORD) -1;

	if(dwPrevSpeed == speed)
		return;

	dwPrevSpeed = speed;

	// SPEED_MIN    =  0 = 0.50 Mhz
	// SPEED_NORMAL = 10 = 1.00 Mhz
	//                20 = 2.00 MHz
	// SPEED_MAX-1  = 39 = 3.90 Mhz
	// SPEED_MAX    = 40 = ???? MHz (run full-speed, /g_fCurrentCLK6502/ is ignored)

	if(speed < SPEED_NORMAL)
		g_fMHz = 0.5 + (double)speed * 0.05;
	else
		g_fMHz = (double)speed / 10.0;

	g_fCurrentCLK6502 = CLK_6502 * g_fMHz;

	//
	// Now re-init modules that are dependent on /g_fCurrentCLK6502/
	//

	SpkrReinitialize();
}

//===========================================================================
void EnterMessageLoop()
{
	MSG message;
	while (GetMessage(&message,0,0,0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
		Timer_StopTimer();
		while ((mode == MODE_RUNNING) || (mode == MODE_STEPPING))
		{
			if (PeekMessage(&message,0,0,0,PM_REMOVE))
			{
				if (message.message == WM_QUIT)
					return;
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else if (mode == MODE_STEPPING)
				DebugContinueStepping();
			else
			{
				ContinueExecution();
				if (fullspeed)
					ContinueExecution();
			}
		}
	}
	while (PeekMessage(&message,0,0,0,PM_REMOVE));
}

//===========================================================================
void GetProgramDirectory()
{
	int loop;

	GetModuleFileName((HINSTANCE)0, progdir, MAX_PATH);
	progdir[MAX_PATH-1] = 0;
	loop = strlen(progdir);
	while (loop--)
		if ((progdir[loop] == '\\') ||
					(progdir[loop] == ':'))
		{
			progdir[loop+1] = 0;
			break;
		}
	GetTempPath(MAX_PATH, tempdir);
}

//===========================================================================
void LoadConfiguration()
{

#define LOAD(a,b) RegLoadValue(CONFIG,a,1,(unsigned long *)b);
	LOAD(EMUJOY,    &joytype);
	LOAD(EMUSOM,    &soundtype);
	LOAD(EMUSLOT,   &NewSlotAux);
	LOAD(EMUVELCPU, &speed);
	LOAD(EMUDISCO,  (DWORD *)&enhancedisk);
	LOAD(EMUVIDEO,  &videotype);
	LOAD(EMUVIDEOM, &monochrome);
	LOAD(EMUSCANLINES,  &ScanLines); // Scan Lines
	LOAD(EMUIMPRESSORA, &ImpressoraPorta);
	LOAD(EMUCOLARRAPIDO, &ColarRapido);

#undef LOAD
	SetCurrentCLK6502();
}

//===========================================================================
void RegisterExtensions ()
{
#ifdef REGISTRAREXTENSOES
	char  command[MAX_PATH];
	char  icon[MAX_PATH];

	GetModuleFileName((HMODULE)0, command, MAX_PATH);
	command[MAX_PATH-1] = 0;
	sprintf(icon,"%s,1",(char*)command);
	strcat(command," %1");

#define CHAVE "ImagemDiscoTK2000"
//	RegSetValue(HKEY_CLASSES_ROOT,".do" ,REG_SZ,CHAVE, 10);
	RegSetValue(HKEY_CLASSES_ROOT,".dsk",REG_SZ,CHAVE, 10);
//	RegSetValue(HKEY_CLASSES_ROOT,".nib",REG_SZ,CHAVE, 10);
//	RegSetValue(HKEY_CLASSES_ROOT,".po" ,REG_SZ,CHAVE, 10);
	RegSetValue(HKEY_CLASSES_ROOT,CHAVE, REG_SZ,CHAVE, 21);
	RegSetValue(HKEY_CLASSES_ROOT,CHAVE"\\DefaultIcon",
				REG_SZ,icon,strlen(icon)+1);
	RegSetValue(HKEY_CLASSES_ROOT,CHAVE"\\shell\\open\\command",
				REG_SZ,command,strlen(command)+1);
#undef CHAVE
#endif
}

//===========================================================================
void Inicializar1()
{
	KeybInicia();
	ImageInitialize();
	DiskInitialize();
	ImpressoraInicializa();
//	HDInicializa();
//	IDEInicializa();
	CreateColorMixMap();
}

//===========================================================================
void Inicializar2()
{
	Som_Inicializa();
	SpkrInitialize();
	TapeInicializa();
}

//===========================================================================
void Finalizar(int restart)
{
	if (!restart)
	{
		DiskDestroy();
		ImageDestroy();
		ImpressoraFinaliza();
	}
	CpuDestroy();
	MemDestroy();
	TapeFinaliza();
	SpkrDestroy();
	VideoDestroy();
	Timer_UninitTimer();
	KeybFinaliza();
}

//===========================================================================
int APIENTRY WinMain (HINSTANCE passinstance,
					  HINSTANCE previnstance,
					  LPSTR     lpCmdLine,
					  int       nCmdShow)
{
#ifdef CPUDEBUG
	char nomearq[MAX_PATH];
#endif

	char imagefilename[MAX_PATH];

	// DO ONE-TIME INITIALIZATION
	instance = passinstance;

	// Initialize COM
	CoInitialize( NULL );

	GdiSetBatchLimit(512);
	GetProgramDirectory();
	RegisterExtensions();
	FrameRegisterClass();
	Inicializar1();

	strcpy(imagefilename, progdir);
	strcat(imagefilename, NOMEARQTKDOS);

	if (lpCmdLine[0] != '\0')
	{
		CharLowerBuff(&lpCmdLine[strlen(lpCmdLine)-3],3);
		strcpy(imagefilename, lpCmdLine);
	}

#ifdef CPUDEBUG
	strcpy(nomearq, progdir);
	strcat(nomearq, "debugCPU.txt");
	DeleteFile(nomearq);
	arquivocpu = fopen(nomearq, "wb");
#endif


	do
	{
		// DO INITIALIZATION THAT MUST BE REPEATED FOR A RESTART
		restart = 0;
		mode    = MODE_LOGO;
		LoadConfiguration();
		DebugInitialize();
		JoyInitialize();
		MemInitialize();
		VideoInitialize();
		Timer_InitTimer();

		FrameCreateWindow();
		MemAtualizaSlotAux();

		if (imagefilename[0] != '\0')
		{
			DiskInsert(0, imagefilename, FALSE, FALSE);
			imagefilename[0] = '\0';
		}

		// ENTER THE MAIN MESSAGE LOOP
		EnterMessageLoop();
	}
	while (restart);

#ifdef CPUDEBUG
	//CloseHandle(arquivocpu);
	fclose(arquivocpu);
#endif

	// Release COM
	CoUninitialize();

	return 0;
}



// EOF