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

// Emula um joystick

#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_joystick.h"
#include "tk_cpu.h"
#include "tk_memoria.h"

//#define  BUTTONTIME       5000

#define  DEVICE_NONE      0
#define  DEVICE_JOYSTICK  1
#define  DEVICE_KEYBOARD  2
#define  DEVICE_MOUSE     3

#define  MODE_NONE        0
#define  MODE_STANDARD    1
#define  MODE_CENTERING   2
#define  MODE_SMOOTH      3

typedef struct _joyinforec
{
	int device;
	int mode;
} joyinforec, *joyinfoptr;

joyinforec joyinfo[5]   = {{DEVICE_NONE,MODE_NONE},
                           {DEVICE_JOYSTICK,MODE_STANDARD},
                           {DEVICE_KEYBOARD,MODE_STANDARD},
                           {DEVICE_KEYBOARD,MODE_CENTERING},
                           {DEVICE_MOUSE,MODE_STANDARD}};
BOOL       keydown[9]   = {0,0,0,0,0,0,0,0,0};
POINT      keyvalue[9]  = {{0,255},{127,255},{255,255},
                           {0,127},{127,127},{255,127},
                           {0,0  },{127,0  },{255,0  }};

// Variáveis:
BOOL  joybutton[3]   = {0,0,0};
int   joyshrx        = 8;
int   joyshry        = 8;
int   joysubx        = 0;
int   joysuby        = 0;
DWORD joytype        = 1;
int   xpos           = 127;
int   ypos           = 127;

//===========================================================================
void CheckJoystick()
{
	static DWORD lastcheck = 0;
	DWORD currtime = GetTickCount();

	if ((currtime-lastcheck >= 10) || joybutton[0] || joybutton[1])
	{
		JOYINFO info;

		lastcheck = currtime;
		if (joyGetPos(JOYSTICKID1, &info) == JOYERR_NOERROR)
		{
			joybutton[0] = ((info.wButtons & JOY_BUTTON1) != 0);
			joybutton[1] = ((info.wButtons & JOY_BUTTON2) != 0);
			joybutton[2] = ((info.wButtons & JOY_BUTTON3) != 0);
			xpos = (info.wXpos-joysubx) >> joyshrx;
			ypos = (info.wYpos-joysuby) >> joyshry;
		}
	}
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void JoyInitialize ()
{
	UINT xrange;
	UINT yrange;

	if (joyinfo[joytype].device == DEVICE_JOYSTICK)
	{
		JOYCAPS caps;
		if (joyGetDevCaps(JOYSTICKID1,&caps,sizeof(JOYCAPS)) == JOYERR_NOERROR)
		{
			joyshrx = 0;
			joyshry = 0;
			joysubx = (int)caps.wXmin;
			joysuby = (int)caps.wYmin;
			xrange  = caps.wXmax-caps.wXmin;
			yrange  = caps.wYmax-caps.wYmin;
			while (xrange > 256)
			{
				xrange >>= 1;
				++joyshrx;
			}
			while (yrange > 256)
			{
				yrange >>= 1;
				++joyshry;
			}
		}
		else
			joytype = 3;
	}
}

//===========================================================================
void JoyReset()
{
	int loop = 0;
	while (loop < 9)
		keydown[loop++] = 0;
}

//===========================================================================
void JoySetButton (int number, BOOL down)
{
	if (number > 1)
		return;
	joybutton[number] = down;
}

//===========================================================================
BOOL JoySetEmulationType (HWND window, DWORD newtype)
{
	if (joyinfo[newtype].device == DEVICE_JOYSTICK)
	{
		JOYCAPS caps;
		if (joyGetDevCaps(JOYSTICKID1,&caps,sizeof(JOYCAPS)) != JOYERR_NOERROR)
		{
			MessageBox(window,
					TEXT("O emulador não conseguiu ler o joystick do seu computador.\n")
					TEXT("Se o joystick estiver configurado corretamente,\n")
					TEXT("verifique se o cabo do joystick está encaixado corretamente\n")
					TEXT("e verifique se o driver do joystick está instalado."),
					TEXT("Configuração"),
					MB_ICONEXCLAMATION | MB_SETFOREGROUND);
			return 0;
		}
	}
	else if ((joyinfo[newtype].device == DEVICE_MOUSE) &&
			(joyinfo[joytype].device != DEVICE_MOUSE))
		MessageBox(window,
				TEXT("Para começar a emular o joystick com seu mouse, clique ")
				TEXT("na tela emulada com o botão esquerdo. Durante a emulação ")
				TEXT("o mouse não poderá ser usado para outros fins, para voltar ")
				TEXT("a usá-lo, clique com o botão esquerdo enquanto você segura ")
				TEXT("a tecla CTRL."),
				TEXT("Configuração"),
				MB_ICONINFORMATION | MB_SETFOREGROUND);
	joytype = newtype;
	JoyInitialize();
	JoyReset();
	return 1;
}

//===========================================================================
void JoySetPosition (int xvalue, int xrange, int yvalue, int yrange)
{
	xpos = (xvalue*255)/xrange;
	ypos = (yvalue*255)/yrange;
}
 
//===========================================================================
void JoyUpdatePosition (DWORD cycles)
{
	//
}

//===========================================================================
BOOL JoyUsingMouse ()
{
	return (joyinfo[joytype].device == DEVICE_MOUSE);
}

//===========================================================================
BOOL JoyProcessKey (int virtkey, BOOL extended, BOOL down, BOOL autorep)
{
	BOOL keychange;

	if ((joyinfo[joytype].device != DEVICE_KEYBOARD))
		return 0;
	keychange = !extended;
	if (!extended)
	{
		if ((virtkey >= VK_NUMPAD1) && (virtkey <= VK_NUMPAD9))
			keydown[virtkey-VK_NUMPAD1] = down;
		else
			switch (virtkey)
			{
				case VK_END:     keydown[ 0] = down;  break; // 1
				case VK_DOWN:    keydown[ 1] = down;  break; // 2
				case VK_NEXT:    keydown[ 2] = down;  break; // 3
				case VK_LEFT:    keydown[ 3] = down;  break; // 4
				case VK_CLEAR:   keydown[ 4] = down;  break; // 5
				case VK_RIGHT:   keydown[ 5] = down;  break; // 6
				case VK_HOME:    keydown[ 6] = down;  break; // 7
				case VK_UP:      keydown[ 7] = down;  break; // 8
				case VK_PRIOR:   keydown[ 8] = down;  break; // 9
				case VK_NUMPAD0:
				case VK_INSERT:  joybutton[0] = down; break;
				case VK_DECIMAL:
				case VK_DELETE:  joybutton[1] = down; break;
				default:         keychange = 0;       break;
			}
		if (keychange)
			if ((down && !autorep) || (joyinfo[joytype].mode == MODE_CENTERING))
			{
				int xkeys  = 0;
				int ykeys  = 0;
				int xtotal = 0;
				int ytotal = 0;
				int keynum = 0;
				while (keynum < 9)
				{
					if (keydown[keynum])
					{
						if ((keynum % 3) != 1)
						{
							xkeys++;
							xtotal += keyvalue[keynum].x;
						}
						if ((keynum / 3) != 1)
						{
							ykeys++;
							ytotal += keyvalue[keynum].y;
						}
					}
					keynum++;
				}
				if (xkeys)
					xpos = xtotal / xkeys;
				else
					xpos = 127;
				if (ykeys)
					ypos = ytotal / ykeys;
				else
					ypos = 127;
		}
	}
	return keychange;
}

//===========================================================================
BYTE JoyVerificaTecla(int linha)
{
	BYTE result = 0;

	if (joyinfo[joytype].device == DEVICE_JOYSTICK)
		CheckJoystick();
	if (linha == 7 && joybutton[0])
		result |= 0x10;
	if (linha == 6)
	{
		if (joybutton[1])
			result |= 0x10;
		if (ypos < 30)
			result |= 0x01;
	}
	if (linha == 5)
	{
		if (ypos > 225)
			result |= 0x01;
	}
	if (linha == 4)
	{
		if (xpos > 225)
			result |= 0x01;
	}
	if (linha == 3)
	{
		if (xpos < 30)
			result |= 0x01;
	}
	return result;
}

// EOF