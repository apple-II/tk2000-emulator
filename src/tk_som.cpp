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

// Funções para manipulação do registro do Windows

#include <crtdbg.h>
#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_video.h"
#include "tk_janela.h"
#include "tk_som_nucleo.h"
#include "tk_cpu.h"
#include "tk_memoria.h"

// Notes:
//
// 23.191 Apple CLKs == 44100Hz (CLK_6502/44100)
//
// The speaker's wave output drives how much 6502 emulation is done in real-time, eg:
// If the speaker's wave buffer is running out of sample-data, then more 6502 cycles
// need to be executed to top-up the wave buffer.
// This is in contrast to the AY8910 voices, which can simply generate more data if
// their buffers are running low.
//

// Definições
#define  SOUND_NONE    0
#define  SOUND_DIRECT  1
#define  SOUND_SMART   2
#define  SOUND_WAVE    3


// Variáveis
static short*	g_pSpeakerBuffer = NULL;
bool            g_bSpkrToggleFlag = false;

// Globals (SOUND_WAVE)
const short		SPKR_DATA_INIT = (short)0x8000;
static short	g_nSpeakerData	= SPKR_DATA_INIT;
static UINT		g_nBufferIdx	= 0;
static short*	g_pRemainderBuffer = NULL;
static UINT		g_nRemainderBufferSize;		// Setup in SpkrInitialize()
static UINT		g_nRemainderBufferIdx;		// Setup in SpkrInitialize()

// Application-wide globals:
DWORD			soundtype		= SOUND_WAVE;
double		    g_fClksPerSpkrSample;		// Setup in SetClksPerSpkrSample()

// Globals
static DWORD	lastcyclenum	= 0;
static DWORD	toggles			= 0;
static unsigned __int64	g_nSpkrQuietCycleCount = 0;
static unsigned __int64 g_nSpkrLastCycle = 0;

// Globals (SOUND_DIRECT/SOUND_SMART)
static BOOL		directio		= 0;
static DWORD	lastdelta[2]	= {0,0};
static DWORD	quietcycles		= 0;
static DWORD	soundeffect		= 0;
static DWORD	totaldelta		= 0;

//===========================================================================
static void DisplayBenchmarkResults()
{
	char buffer[64];

	DWORD totaltime = GetTickCount()-extbench;
	VideoRedrawScreen();
	sprintf(buffer,
				"Este benchmark rodou em %u.%02u segundos.",
				(unsigned)(totaltime / 1000),
				(unsigned)((totaltime / 10) % 100));
	MessageBox((HWND)framewindow,
				buffer,
				"Resultados do Benchmark",
				MB_ICONINFORMATION | MB_SETFOREGROUND);
}

//===========================================================================
static void InternalBeep (DWORD frequency, DWORD duration)
{
#ifdef _X86_
	if (directio)
		if (duration)
		{
			frequency = 1193180/frequency;
			__asm
			{
				push eax
				mov  al,0B6h
				out  43h,al
				mov  eax,frequency
				out  42h,al
				mov  al,ah
				out  42h,al
				in   al,61h
				or   al,3
				out  61h,al
				pop  eax
			}
		}
		else
			__asm
			{
				push eax
				in   al,61h
				and  al,0FCh
				out  61h,al
				pop  eax
			}
	else
#endif
		Beep(frequency,duration);
}

//===========================================================================
static void InternalClick ()
{
#ifdef _X86_
	if (directio)
		__asm
		{
			push eax
			in   al,0x61
			xor  al,2
			out  0x61,al
			pop  eax
		}
	else
	{
#endif
		Beep(37,(DWORD)-1);
		Beep(0,0);
#ifdef _X86_
	}
#endif
}

//===========================================================================
static void SetClksPerSpkrSample()
{
	// 23.191 clks for 44.1Khz (when 6502 CLK=1.0Mhz)
	g_fClksPerSpkrSample = g_fCurrentCLK6502 / (double)SPKR_SAMPLE_RATE;
}

//===========================================================================
static void InitRemainderBuffer()
{
	delete [] g_pRemainderBuffer;

	SetClksPerSpkrSample();
	g_nRemainderBufferSize = (UINT) g_fClksPerSpkrSample + 1;

	g_pRemainderBuffer = new short [g_nRemainderBufferSize];
	memset(g_pRemainderBuffer, 0, g_nRemainderBufferSize);

	g_nRemainderBufferIdx = 0;
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void SpkrDestroy ()
{
	if(soundtype == SOUND_WAVE)
	{
		delete [] g_pSpeakerBuffer;
		delete [] g_pRemainderBuffer;

		g_pSpeakerBuffer = NULL;
		g_pRemainderBuffer = NULL;
	}
	else
	{
		InternalBeep(0,0);
	}
}

//===========================================================================
void SpkrInitialize ()
{
	if(g_fh)
	{
		fprintf(g_fh, "Spkr Config: soundtype = %d ",soundtype);
		switch(soundtype)
		{
			case SOUND_NONE:   fprintf(g_fh, "(NONE)\n"); break;
			case SOUND_DIRECT: fprintf(g_fh, "(DIRECT)\n"); break;
			case SOUND_SMART:  fprintf(g_fh, "(SMART)\n"); break;
			case SOUND_WAVE:   fprintf(g_fh, "(WAVE)\n"); break;
			default:           fprintf(g_fh, "(UNDEFINED!)\n"); break;
		}
	}

	if (soundtype == SOUND_WAVE)
	{
		InitRemainderBuffer();

		g_pSpeakerBuffer = new short [SPKR_SAMPLE_RATE];	// Buffer can hold a max of 1 seconds worth of samples
	}

	// IF NONE IS, THEN DETERMINE WHETHER WE HAVE DIRECT ACCESS TO THE PC SPEAKER PORT
	if (soundtype != SOUND_WAVE)	// *** TO DO: Need way of determining if DirectX init failed ***
	{
		if (soundtype == SOUND_WAVE)
			soundtype = SOUND_SMART;
#ifdef _X86_
		_try
		{
			__asm
			{
				in  al,0x61
				xor al,2
				out 0x61,al
				xor al,2
				out 0x61,al
			}
			directio = 1;
		}
		_except (EXCEPTION_EXECUTE_HANDLER)
		{
			directio = 0;
		}
#else
		directio = 0;
#endif
		if ((!directio) && (soundtype == SOUND_DIRECT))
			soundtype = SOUND_SMART;
	}
}

//===========================================================================

// NB. Called when /g_fCurrentCLK6502/ changes
void SpkrReinitialize ()
{
	if (soundtype == SOUND_WAVE)
	{
		InitRemainderBuffer();
	}
}

//===========================================================================

void SpkrReset()
{
	InitRemainderBuffer();
	Som_SubmitWaveBuffer(NULL, 0);
	Som_Demute();
}

//===========================================================================
BOOL SpkrSetEmulationType(HWND window, DWORD newtype)
{
	if (soundtype != SOUND_NONE)
		SpkrDestroy();
	soundtype = newtype;
	if (soundtype != SOUND_NONE)
		SpkrInitialize();
	if (soundtype != newtype)
		switch (newtype)
		{

			case SOUND_DIRECT:
				MessageBox(window,
							"Emulação direta não é possível por causa do "
							"sistema operacional que você está usando não "
							"permite acesso direto ao speaker.",
							"Configuração",
							MB_ICONEXCLAMATION | MB_SETFOREGROUND);
				return 0;

			case SOUND_WAVE:
				MessageBox(window,
							"O emulador não conseguiu inicializar um dispositivo "
							"de saída de som. Verifique se você tem uma placa de "
							"som instalada e se ela está configurada corretamente.\n"
							"Verifique também se nenhum outro programa não está "
							"usando a placa de som.",
							"Configuração",
							MB_ICONEXCLAMATION | MB_SETFOREGROUND);
				return 0;

	}
	return 1;
}

//===========================================================================
static void InitRemainderBuffer(UINT nCyclesRemaining)
{
	if(nCyclesRemaining == 0)
		return;

	for(g_nRemainderBufferIdx=0; g_nRemainderBufferIdx<nCyclesRemaining; g_nRemainderBufferIdx++)
		g_pRemainderBuffer[g_nRemainderBufferIdx] = g_nSpeakerData;

	_ASSERT(g_nRemainderBufferIdx < g_nRemainderBufferSize);
}

//===========================================================================
static void UpdateRemainderBuffer(ULONG* pnCycleDiff)
{
	if(g_nRemainderBufferIdx)
	{
		while((g_nRemainderBufferIdx < g_nRemainderBufferSize) && *pnCycleDiff)
		{
			g_pRemainderBuffer[g_nRemainderBufferIdx] = g_nSpeakerData;
			g_nRemainderBufferIdx++;
			(*pnCycleDiff)--;
		}

		if(g_nRemainderBufferIdx == g_nRemainderBufferSize)
		{
			g_nRemainderBufferIdx = 0;
			signed long nSampleMean = 0;
			for(UINT i=0,nCnt=0; i<g_nRemainderBufferSize; i++)
				nSampleMean += (signed long) g_pRemainderBuffer[i];
			nSampleMean /= (signed long) g_nRemainderBufferSize;

			if(g_nBufferIdx < SPKR_SAMPLE_RATE-1)
				g_pSpeakerBuffer[g_nBufferIdx++] = (short) nSampleMean;
		}
	}
}

//===========================================================================
static void UpdateSpkr()
{
	ULONG nCycleDiff = (ULONG) (g_nCumulativeCycles - g_nSpkrLastCycle);

	UpdateRemainderBuffer(&nCycleDiff);

	ULONG nNumSamples = (ULONG) ((double)nCycleDiff / g_fClksPerSpkrSample);

	ULONG nCyclesRemaining = (ULONG) ((double)nCycleDiff - (double)nNumSamples * g_fClksPerSpkrSample);

	while((nNumSamples--) && (g_nBufferIdx < SPKR_SAMPLE_RATE-1))
		g_pSpeakerBuffer[g_nBufferIdx++] = g_nSpeakerData;

	InitRemainderBuffer(nCyclesRemaining);	// Partially fill 1Mhz sample buffer

	g_nSpkrLastCycle = g_nCumulativeCycles;
}

//===========================================================================
// Called by emulation code when Speaker I/O reg is accessed
//
BYTE __stdcall SpkrToggle (WORD, BYTE address, BYTE write, BYTE)
{
	g_bSpkrToggleFlag = true;

	Som_Demute();
	if (extbench)
	{
		DisplayBenchmarkResults();
		extbench = 0;
	}

	if (soundtype == SOUND_WAVE)
	{
		UpdateSpkr();
		g_nSpeakerData = ~g_nSpeakerData;
	}
	else if (soundtype != SOUND_NONE)
	{

		// IF WE ARE CURRENTLY PLAYING A SOUND EFFECT OR ARE IN DIRECT
		// EMULATION MODE, TOGGLE THE SPEAKER
		if ((soundeffect > 2) || (soundtype == SOUND_DIRECT))
		{
			if (directio)
			{
				__asm
				{
					push eax
					in   al,0x61
					xor  al,2
					out  0x61,al
					pop  eax
				}
			}
			else
			{
				Beep(37,(DWORD)-1);
				Beep(0,0);
			}
		}


		// SAVE INFORMATION ABOUT THE FREQUENCY OF SPEAKER TOGGLING FOR POSSIBLE
		// LATER USE BY SOUND AVERAGING
		if (lastcyclenum)
		{
			toggles++;
			DWORD delta = cyclenum-lastcyclenum;

			// DETERMINE WHETHER WE ARE PLAYING A SOUND EFFECT
			if (directio &&
					((delta < 250) ||
					(lastdelta[0] && lastdelta[1] &&
					(delta-lastdelta[0] > 250) && (lastdelta[0]-delta > 250) &&
					(delta-lastdelta[1] > 250) && (lastdelta[1]-delta > 250))))
				soundeffect = MIN(35,soundeffect+2);

			lastdelta[1] = lastdelta[0];
			lastdelta[0] = delta;
			totaldelta  += delta;
		}
		lastcyclenum = cyclenum;

	}

	return MemReturnRandomData(1);
}

//===========================================================================
// Called by ContinueExecution()
void SpkrUpdate (DWORD totalcycles)
{
	if(!g_bSpkrToggleFlag)
	{
		if(!g_nSpkrQuietCycleCount)
		{
			g_nSpkrQuietCycleCount = g_nCumulativeCycles;
		}
		else if(g_nCumulativeCycles - g_nSpkrQuietCycleCount > (unsigned __int64)g_fCurrentCLK6502/5)
		{
			// After 0.2 sec of Apple time, mute spkr voice
			Som_Mute();
		}
	}
	else
	{
		g_nSpkrQuietCycleCount = 0;
		g_bSpkrToggleFlag = false;
	}

	//

	if (soundtype == SOUND_WAVE)
	{
		UpdateSpkr();

		if(fullspeed)
		{
			g_nBufferIdx = 0;
		}
		else
		{
			ULONG nSamplesUsed = Som_SubmitWaveBuffer(g_pSpeakerBuffer, g_nBufferIdx);
			_ASSERT(nSamplesUsed <= g_nBufferIdx);
			memmove(g_pSpeakerBuffer, &g_pSpeakerBuffer[nSamplesUsed], g_nBufferIdx-nSamplesUsed);
			g_nBufferIdx -= nSamplesUsed;
		}
	}
	else
	{

		// IF WE ARE NOT PLAYING A SOUND EFFECT, PERFORM FREQUENCY AVERAGING
		static DWORD currenthertz = 0;
		static BOOL  lastfull     = 0;
		static DWORD lasttoggles  = 0;
		static DWORD lastval      = 0;
		if ((soundeffect > 2) || (soundtype == SOUND_DIRECT))
		{
			lastval = 0;
			if (currenthertz && (soundeffect > 4))
			{
				InternalBeep(0,0);
				currenthertz = 0;
			}
		}
		else if (toggles && totaldelta)
		{
			DWORD newval = 1000000*toggles/totaldelta;
			if (lastval && lastfull &&
					(newval-currenthertz > 50) &&
					(currenthertz-newval > 50))
			{
				InternalBeep(newval,(DWORD)-1);
				currenthertz = newval;
				lasttoggles  = 0;
			}
			lastfull     = (totaldelta+((totaldelta/toggles) << 1) >= totalcycles);
			lasttoggles += toggles;
			lastval      = newval;
		}
		else if (currenthertz)
		{
			InternalBeep(0,0);
			currenthertz = 0;
			lastfull     = 0;
			lasttoggles  = 0;
			lastval      = 0;
		}
		else if (lastval)
		{
			currenthertz = (lasttoggles > 4) ? lastval : 0;
			if (currenthertz)
				InternalBeep(lastval,(DWORD)-1);
			else
				InternalClick();
			lastfull     = 0;
			lasttoggles  = 0;
			lastval      = 0;
		}

		// RESET THE FREQUENCY GATHERING VARIABLES
		lastcyclenum = 0;
		lastdelta[0] = 0;
		lastdelta[1] = 0;
		quietcycles  = toggles ? 0 : (quietcycles+totalcycles);
		toggles      = 0;
		totaldelta   = 0;
		if (soundeffect)
			soundeffect--;

	}
}

// EOF