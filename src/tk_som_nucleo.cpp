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

// Funções do Som

// Core sound related functionality

#include <crtdbg.h>
#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_som_nucleo.h"
#include "tk_janela.h"
#include "tk_som.h"

//-----------------------------------------------------------------------------

// Definições
#define MAX_SOUND_DEVICES 10

// Variáveis
static char *sound_devices[MAX_SOUND_DEVICES];
static GUID sound_device_guid[MAX_SOUND_DEVICES];
static int num_sound_devices = 0;
static bool g_bSpkrMuted = false;

static LPDIRECTSOUND g_lpDS = NULL;

//-------------------------------------

bool g_bDSAvailable = false;
static bool g_bSPKRAvailable = false;


static const unsigned short g_nSPKR_NumChannels = 1;
static VOICE SpeakerVoice = {0};
static const DWORD g_dwDSSpkrBufferSize = 16 * 1024 * sizeof(short) * g_nSPKR_NumChannels;



// Funções Internas
//-----------------------------------------------------------------------------

static BOOL CALLBACK DSEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName,  LPVOID lpContext)
{
	int i = num_sound_devices;
	if(i == MAX_SOUND_DEVICES)
		return TRUE;
	if(lpGUID != NULL)
		memcpy(&sound_device_guid[i], lpGUID, sizeof (GUID));
	sound_devices[i] = strdup(lpszDesc);

	if(g_fh) fprintf(g_fh, "%d: %s - %s\n",i,lpszDesc,lpszDrvName);

	num_sound_devices++;
	return TRUE;
}

//-----------------------------------------------------------------------------

#ifdef _DEBUG
static char *DirectSound_ErrorText (HRESULT error)
{
    switch( error )
    {
    case DSERR_ALLOCATED:
        return "Allocated";
    case DSERR_CONTROLUNAVAIL:
        return "Control Unavailable";
    case DSERR_INVALIDPARAM:
        return "Invalid Parameter";
    case DSERR_INVALIDCALL:
        return "Invalid Call";
    case DSERR_GENERIC:
        return "Generic";
    case DSERR_PRIOLEVELNEEDED:
        return "Priority Level Needed";
    case DSERR_OUTOFMEMORY:
        return "Out of Memory";
    case DSERR_BADFORMAT:
        return "Bad Format";
    case DSERR_UNSUPPORTED:
        return "Unsupported";
    case DSERR_NODRIVER:
        return "No Driver";
    case DSERR_ALREADYINITIALIZED:
        return "Already Initialized";
    case DSERR_NOAGGREGATION:
        return "No Aggregation";
    case DSERR_BUFFERLOST:
        return "Buffer Lost";
    case DSERR_OTHERAPPHASPRIO:
        return "Other Application Has Priority";
    case DSERR_UNINITIALIZED:
        return "Uninitialized";
    case DSERR_NOINTERFACE:
        return "No Interface";
    default:
        return "Unknown";
    }
}
#endif

//-----------------------------------------------------------------------------

bool DSGetLock(LPDIRECTSOUNDBUFFER pVoice, DWORD dwOffset, DWORD dwBytes,
					  SHORT** ppDSLockedBuffer0, DWORD* pdwDSLockedBufferSize0,
					  SHORT** ppDSLockedBuffer1, DWORD* pdwDSLockedBufferSize1)
{
	DWORD nStatus;
	HRESULT hr = pVoice->GetStatus(&nStatus);
	if(hr != DS_OK)
		return false;

	if(nStatus & DSBSTATUS_BUFFERLOST)
	{
		do
		{
			hr = pVoice->Restore();
			if(hr == DSERR_BUFFERLOST)
				Sleep(10);
		}
		while(hr != DS_OK);
	}

	// Get write only pointer(s) to sound buffer
	if(dwBytes == 0)
	{
		if(FAILED(hr = pVoice->Lock(0, 0,
								(void**)ppDSLockedBuffer0, pdwDSLockedBufferSize0,
								(void**)ppDSLockedBuffer1, pdwDSLockedBufferSize1,
								DSBLOCK_ENTIREBUFFER)))
			return false;
	}
	else
	{
		if(FAILED(hr = pVoice->Lock(dwOffset, dwBytes,
								(void**)ppDSLockedBuffer0, pdwDSLockedBufferSize0,
								(void**)ppDSLockedBuffer1, pdwDSLockedBufferSize1,
								0)))
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------

HRESULT DSGetSoundBuffer(VOICE* pVoice, DWORD dwFlags, DWORD dwBufferSize, DWORD nSampleRate, int nChannels)
{
	WAVEFORMATEX wavfmt;
	DSBUFFERDESC dsbdesc;

	wavfmt.wFormatTag = WAVE_FORMAT_PCM;
	wavfmt.nChannels = nChannels;
	wavfmt.nSamplesPerSec = nSampleRate;
	wavfmt.wBitsPerSample = 16;
	wavfmt.nBlockAlign = wavfmt.nChannels==1 ? 2 : 4;
	wavfmt.nAvgBytesPerSec = wavfmt.nBlockAlign * wavfmt.nSamplesPerSec;

	memset (&dsbdesc, 0, sizeof (dsbdesc));
	dsbdesc.dwSize = sizeof (dsbdesc);
	dsbdesc.dwBufferBytes = dwBufferSize;
	dsbdesc.lpwfxFormat = &wavfmt;
	dsbdesc.dwFlags = dwFlags | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS;

	return g_lpDS->CreateSoundBuffer(&dsbdesc, &pVoice->lpDSBvoice, NULL);
}

//-----------------------------------------------------------------------------

bool DSZeroVoiceBuffer(PVOICE Voice, char* pszDevName, DWORD dwBufferSize)
{
	DWORD dwDSLockedBufferSize = 0;    // Size of the locked DirectSound buffer
	SHORT* pDSLockedBuffer;


	HRESULT hr = Voice->lpDSBvoice->Stop();
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "%s: DSStop failed (%08X)\n",pszDevName,hr);
		return false;
	}

	hr = DSGetLock(Voice->lpDSBvoice, 0, 0, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, 0);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "%s: DSGetLock failed (%08X)\n",pszDevName,hr);
		return false;
	}

	memset(pDSLockedBuffer, 0x00, dwBufferSize);

	hr = Voice->lpDSBvoice->Play(0,0,DSBPLAY_LOOPING);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "%s: DSPlay failed (%08X)\n",pszDevName,hr);
		return false;
	}

	hr = Voice->lpDSBvoice->Unlock((void*)pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "%s: DSUnlock failed (%08X)\n",pszDevName,hr);
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------

bool DSInit()
{
	if(g_bDSAvailable)
		return true;		// Already initialised successfully

	HRESULT hr = DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc, NULL);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "DSEnumerate failed (%08X)\n",hr);
		return false;
	}

	if(g_fh)
	{
		fprintf(g_fh, "Number of sound devices = %d\n",num_sound_devices);
	}


	bool bCreatedOK = false;
	for(int x=0; x<num_sound_devices; x++)
	{
		hr = DirectSoundCreate(&sound_device_guid[x], &g_lpDS, NULL);
		if(SUCCEEDED(hr))
		{
			if(g_fh) fprintf(g_fh, "DSCreate succeeded for sound device #%d\n",x);
			bCreatedOK = true;
			break;
		}

		if(g_fh) fprintf(g_fh, "DSCreate failed for sound device #%d (%08X)\n",x,hr);
	}
	if(!bCreatedOK)
	{
		if(g_fh) fprintf(g_fh, "DSCreate failed for all sound devices\n");
		return false;
	}

	hr = g_lpDS->SetCooperativeLevel((HWND)framewindow, DSSCL_NORMAL);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "SetCooperativeLevel failed (%08X)\n",hr);
		return false;
	}

#if 0
	DSCAPS DSCaps;
    ZeroMemory(&DSCaps, sizeof(DSBCAPS));
    DSCaps.dwSize = sizeof(DSBCAPS);
	hr = g_lpDS->GetCaps(&DSCaps);
	if(FAILED(hr))
	{
		DirectSound_ErrorText(hr);
		return false;
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------

void DSUninit()
{
	// Nothing
}

//-----------------------------------------------------------------------------
static bool Som_DSInit()
{
	//
	// Create single Apple speaker voice
	//

	if(!g_bDSAvailable)
		return false;

	HRESULT hr = DSGetSoundBuffer(&SpeakerVoice, DSBCAPS_CTRLVOLUME, g_dwDSSpkrBufferSize, SPKR_SAMPLE_RATE, 1);
	if(FAILED(hr))
	{
		if(g_fh) fprintf(g_fh, "Spkr: DSGetSoundBuffer failed (%08X)\n",hr);
		return false;
	}

	if(!DSZeroVoiceBuffer(&SpeakerVoice, "Spkr", g_dwDSSpkrBufferSize))
		return false;

	SpeakerVoice.nVolume = DSBVOLUME_MAX;
	SpeakerVoice.bActive = true;

	return true;
}

//-----------------------------------------------------------------------------
static void Som_DSUninit()
{
	if(SpeakerVoice.lpDSBvoice && SpeakerVoice.bActive)
	{
		SpeakerVoice.lpDSBvoice->Stop();
		SpeakerVoice.bActive = false;
	}

	SAFE_RELEASE(SpeakerVoice.lpDSBvoice);
}


// Funções Globais


//-----------------------------------------------------------------------------
void Som_Mute()
{
	if(g_bSpkrMuted)
		return;

	if(SpeakerVoice.bActive)
	{
		SpeakerVoice.lpDSBvoice->SetVolume(DSBVOLUME_MIN);
		g_bSpkrMuted = true;
	}
}

//----------------------------------------------------------------------------
void Som_Demute()
{
	if(!g_bSpkrMuted)
		return;

	if(SpeakerVoice.bActive)
	{
		SpeakerVoice.lpDSBvoice->SetVolume(SpeakerVoice.nVolume);
		g_bSpkrMuted = false;
	}
}

//-----------------------------------------------------------------------------
void Som_Inicializa()
{
	g_bDSAvailable = DSInit();
	g_bSPKRAvailable = Som_DSInit();
}

//-----------------------------------------------------------------------------
void Som_Finaliza()
{
	DSUninit();
	Som_DSUninit();
	g_bDSAvailable = false;
}


//-----------------------------------------------------------------------------

// NB.
// Getting buffer under/overflow errors whilst disk is spinning, as not waiting
// at end of ContinueExecution()

ULONG Som_SubmitWaveBuffer(short* pSpeakerBuffer, ULONG nNumSamples)
{
	if(!SpeakerVoice.bActive)
		return nNumSamples;

	//

	static DWORD dwByteOffset = (DWORD)-1;
	static int nNumSamplesError = 0;


	if(pSpeakerBuffer == NULL)
	{
		// Re-init
		dwByteOffset = (DWORD)-1;
		nNumSamplesError = 0;

		DSZeroVoiceBuffer(&SpeakerVoice, "Spkr", g_dwDSSpkrBufferSize);

		return 0;
	}

	//

	DWORD dwDSLockedBufferSize0, dwDSLockedBufferSize1;
	SHORT *pDSLockedBuffer0, *pDSLockedBuffer1;
	bool bBufferError = false;

	DWORD dwCurrentPlayCursor, dwCurrentWriteCursor;
	HRESULT hr = SpeakerVoice.lpDSBvoice->GetCurrentPosition(&dwCurrentPlayCursor, &dwCurrentWriteCursor);
	if(FAILED(hr))
		return nNumSamples;

	if(dwByteOffset == (DWORD)-1)
	{
		// First time in this func

		dwByteOffset = dwCurrentWriteCursor;
		dwByteOffset += g_dwDSSpkrBufferSize/16;
		dwByteOffset %= g_dwDSSpkrBufferSize;
	}
	else
	{
		// Check that our offset isn't between Play & Write positions

		if(dwCurrentWriteCursor > dwCurrentPlayCursor)
		{
			// |-----PxxxxxW-----|
			if((dwByteOffset > dwCurrentPlayCursor) && (dwByteOffset < dwCurrentWriteCursor))
			{
				dwByteOffset = dwCurrentWriteCursor;
				nNumSamplesError = 0;
				bBufferError = true;
			}
		}
		else
		{
			// |xxW----------Pxxx|
			if((dwByteOffset > dwCurrentPlayCursor) || (dwByteOffset < dwCurrentWriteCursor))
			{
				dwByteOffset = dwCurrentWriteCursor;
				nNumSamplesError = 0;
				bBufferError = true;
			}
		}
	}

	int nBytesRemaining = dwByteOffset - dwCurrentPlayCursor;
	if(nBytesRemaining < 0)
		nBytesRemaining += g_dwDSSpkrBufferSize;

	// Calc correction factor so that play-buffer doesn't under/overflow
	if(nBytesRemaining < g_dwDSSpkrBufferSize / 4)
		nNumSamplesError++;							// < 1/4 of play-buffer remaining (need more data)
	else if(nBytesRemaining > g_dwDSSpkrBufferSize / 2)
		nNumSamplesError--;							// > 1/2 of play-buffer remaining (need less data)
	else
		nNumSamplesError = 0;						// Acceptable amount of data in buffer

	const int nMaxError = 50;	// Cap feedback to +/-nMaxError units
	if(nNumSamplesError < -nMaxError) nNumSamplesError = -nMaxError;
	if(nNumSamplesError >  nMaxError) nNumSamplesError =  nMaxError;
	_ASSERT((nNumSamplesError >= -nMaxError) && (nNumSamplesError <= nMaxError));
	g_nCpuCyclesFeedback = (int) ((double)nNumSamplesError * g_fClksPerSpkrSample);

	ULONG nNumSamplesToUse = nNumSamples;

	if(nNumSamples * sizeof(short) > (UINT)nBytesRemaining)
		nNumSamplesToUse = nBytesRemaining / sizeof(short);

	if(bBufferError)
		pSpeakerBuffer = &pSpeakerBuffer[nNumSamples - nNumSamplesToUse];

	//

	if(!DSGetLock(SpeakerVoice.lpDSBvoice,
						dwByteOffset, (DWORD)nNumSamplesToUse*sizeof(short),
						&pDSLockedBuffer0, &dwDSLockedBufferSize0,
						&pDSLockedBuffer1, &dwDSLockedBufferSize1))
		return nNumSamples;

	memcpy(pDSLockedBuffer0, &pSpeakerBuffer[0], dwDSLockedBufferSize0);
	if(pDSLockedBuffer1)
		memcpy(pDSLockedBuffer1, &pSpeakerBuffer[dwDSLockedBufferSize0/sizeof(short)], dwDSLockedBufferSize1);

	// Commit sound buffer
	hr = SpeakerVoice.lpDSBvoice->Unlock((void*)pDSLockedBuffer0, dwDSLockedBufferSize0,
										(void*)pDSLockedBuffer1, dwDSLockedBufferSize1);

	dwByteOffset = (dwByteOffset + (DWORD)nNumSamplesToUse*sizeof(short)*g_nSPKR_NumChannels) % g_dwDSSpkrBufferSize;

	return bBufferError ? nNumSamples : nNumSamplesToUse;
}


// EOF