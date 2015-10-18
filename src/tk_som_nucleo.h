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

// Definicoes
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

typedef struct
{
	LPDIRECTSOUNDBUFFER lpDSBvoice;
	LPDIRECTSOUNDNOTIFY lpDSNotify;
	bool bActive;
	LONG nVolume;
} VOICE, *PVOICE;


bool DSGetLock(LPDIRECTSOUNDBUFFER pVoice, DWORD dwOffset, DWORD dwBytes,
					  SHORT** ppDSLockedBuffer0, DWORD* pdwDSLockedBufferSize0,
					  SHORT** ppDSLockedBuffer1, DWORD* pdwDSLockedBufferSize1);

HRESULT DSGetSoundBuffer(VOICE* pVoice, DWORD dwFlags, DWORD dwBufferSize, DWORD nSampleRate, int nChannels);

bool DSZeroVoiceBuffer(PVOICE Voice, char* pszDevName, DWORD dwBufferSize);

bool DSInit();
void DSUninit();


extern bool g_bDSAvailable;

// Protótipos
void Som_Mute();
void Som_Demute();
void Som_Inicializa();
void Som_Finaliza();
ULONG Som_SubmitWaveBuffer(short* pSpeakerBuffer, ULONG nNumSamples);


// EOF