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

// Emula a porta da impressora

#include "tk_stdhdr.h"
#include "tk_impressora.h"
#include "tk_main.h"
#include "tk_teclado.h"
#include "tk_memoria.h"
#include "tk_janela.h"

// Definições
#define NOMEARQUIVO	"impressora.txt"

// Variáveis
BYTE	ImpressoraPorta			= 0;
FILE	*ArqImpressora			= NULL;
DWORD	InatividadeImpressora	= 0;
int		MaxH					= 40;

// Protótipos
BOOL ChecarImpressora();

// Funções Internas
//===========================================================================
BOOL ChecarImpressora()
{
	InatividadeImpressora = 0;
	if ((ArqImpressora == NULL) && ImpressoraPorta)
	{
		char nomeporta[255];

		if (ImpressoraPorta != 3)
			sprintf(nomeporta,
#ifdef _WINDOWS
					"LPT%u",
#else // _WINDOWS
					"/dev/lpt%u",
#endif // _WINDOWS
					ImpressoraPorta);
		else
		{
			sprintf(nomeporta,"%s%s",progdir,NOMEARQUIVO);
		}
		ArqImpressora = fopen(nomeporta, ImpressoraPorta != 3 ? "ab+" : "wb+");
		if (ImpressoraPorta == 3)
		{
			fseek(ArqImpressora, SEEK_END, 0);
			//SetFilePointer(ArqImpressora,0,NULL,FILE_END);
		}
	}

	return (ArqImpressora != NULL);
}

// Funções Globais

//===========================================================================
void ImpressoraInicializa()
{
	//
}

//===========================================================================
void ImpressoraFinaliza(void)
{
	if (ArqImpressora != NULL)
	{
		fclose(ArqImpressora);
	}
	ArqImpressora = NULL;
	InatividadeImpressora = 0;
}

//===========================================================================
void ImpressoraReset()
{
	//
}

//===========================================================================
void ImpressoraAtualiza(DWORD totalcycles)
{
	if (ArqImpressora == NULL)
		return;
	InatividadeImpressora += totalcycles;
	if (InatividadeImpressora > 2000000) // 2 segundos
		ImpressoraFinaliza();
	}

//===========================================================================
void ImpressoraDefinePorta(int Porta)
{
	if (ArqImpressora == NULL)
		ImpressoraPorta = Porta;
	else
		FrameMostraMensagemAdvertencia("Você não pode mudar a porta da impressora "
										"enquanto ela estiver em uso.");
}

//===========================================================================
void ImpressoraEnviaChar(BYTE value)
{
	if (ChecarImpressora())
	{
			fwrite(&value, 1, 1, ArqImpressora);
	}
	return;
}

//===========================================================================
BYTE __stdcall ImpressoraStrobe(WORD programcounter, BYTE address, BYTE write, BYTE value)
{
	switch (address)
	{
		case 0x58: ImpressoraEnviaChar(bKBOUT); break;
		case 0x59: break;
	}
	return MemReturnRandomData(0);
}

// EOF