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

#include "tk_stdhdr.h"
#include "tk_registro.h"

#define CHAVE "Software\\TK2000\\VersaoAtual"

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
BOOL RegLoadString (char* section, char* key, BOOL peruser,
                    char* buffer, DWORD chars)
{
	BOOL  success = 0;
	char fullkeyname[256];
	HKEY keyhandle;

	sprintf(fullkeyname, CHAVE "\\%s", (char*)section);
	if (!RegOpenKeyEx((peruser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE),
			fullkeyname,
			0,
			KEY_READ,
			&keyhandle))
	{
		DWORD type;
		DWORD size = chars;
		success = (!RegQueryValueEx(keyhandle,key,0,&type,(LPBYTE)buffer,&size)) && size;
		RegCloseKey(keyhandle);
	}
	return success;
}

//===========================================================================
BOOL RegLoadValue (char* section, char* key, BOOL peruser, DWORD *value)
{
	char buffer[32] = "";

	if (!value)
		return 0;
	if (!RegLoadString(section,key,peruser,buffer,32))
		return 0;
	buffer[31] = 0;
	*value = (DWORD)atoi(buffer);
	return 1;
}

//===========================================================================
void RegSaveString (char* section, char* key, BOOL peruser, char* buffer)
{
	HKEY  keyhandle;
	DWORD disposition;

	char fullkeyname[256];
	sprintf(fullkeyname, CHAVE "\\%s", (char*)section);
	if (!RegCreateKeyEx((peruser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE),
			fullkeyname,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_READ | KEY_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			&keyhandle,
			&disposition))
	{
		RegSetValueEx(keyhandle,
				key,
				0,
				REG_SZ,
				(CONST BYTE *)buffer,
				(strlen(buffer)+1)*sizeof(char));
		RegCloseKey(keyhandle);
	}
}

//===========================================================================
void RegSaveValue (char* section, char* key, BOOL peruser, DWORD value)
{
	char buffer[32] = "";
	//_ultot(value,buffer,10);
	itoa(value,buffer,10);
	RegSaveString(section,key,peruser,buffer);
}

// EOF