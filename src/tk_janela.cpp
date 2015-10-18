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

// Manipula a janela principal e diálogos

#include "tk_stdhdr.h"
#include "tk_janela.h"
#include "tk_main.h"
#include "tk_joystick.h"
#include "tk_registro.h"
#include "tk_teclado.h"
#include "tk_debug.h"
#include "tk_som_nucleo.h"
#include "tk_som.h"
#include "tk_tape.h"
#include "tk_cpu.h"
#include "tk_memoria.h"
#include "tk_video.h"
#include "tk_timer.h"
#include "tk_impressora.h"
#include "tk_disco.h"
#include "../recursos/resource.h"
#include "utilitarios.h"

// Definições
#define	WIDTH_DIALOGO	240
#define	HEIGHT_DIALOGO	120
#define POSX_ROTULO		10
#define POSY_ROTULO		15
#define POSX_CEDIT		166
#define POSY_CEDIT		13
#define POSX_BTNOK      20
#define POSY_BTNOK      55
#define POSX_BTNCANC    130
#define POSY_BTNCANC    55

// Definições
#define STATUSHEIGHT	26
#define STATUSYPOS1		3
#define STATUSYPOS2		STATUSYPOS1 + 11
#define STATUSXPOS1		4
#define STATUSXPOS2		STATUSXPOS1 + 173
#define STATUSXPOS3		STATUSXPOS2 + 173
#define STATUSXPOS4		STATUSXPOS3 + 173
#define VIDEOLEFT		0
#define VIDEOTOP		0
#define VIDEOWIDTH		560
#define VIDEOHEIGHT		384

// Variaveis
BOOL	ativo			= 0;
BOOL	pintando		= 0;
BOOL	usingcursor		= 0;
HWND 	framewindow		= 0;
HDC		framedc			= (HDC)0;
RECT	framerect		= {0,0,0,0};
HFONT	smallfont		= (HFONT)0;
HBRUSH	btnfacebrush    = (HBRUSH)0;
HPEN	btnfacepen      = (HPEN)0;
HPEN	btnhighlightpen = (HPEN)0;
HPEN	btnshadowpen    = (HPEN)0;
HBITMAP	leds[3];

// Variáveis
HWND Janela;
HWND Rotulo;
HWND edtValor;
HWND btnOK;
HWND btnCancelar;
int  BotaoApertado = 0;


char   opsJoystick[]    = "Desabilitado\0"
                          "Joystick do PC\0"
                          "Teclado (padrão)\0"
                          "Teclado (centralizado)\0"
                          "Mouse\0";

char   opsImpressora[]  = "Nenhum\0"
                          "LPT1\0"
                          "LPT2\0"
                          "Arquivo 'Impressora.txt'\0";

char   opsSom[]         = "Desabilitado\0"
                          "PC Speaker (direto)\0"
                          "PC Speaker (translatado)\0"
                          "Placa de Som\0";

char   opsVideo[]       = "Monocromático\0"
                          "Colorido (padrão)\0"
                          "Colorido (otimizado)\0"
                          "Emulação de TV\0";

char   opsExpansao[]    = "Nada\0"
                          "Interface Disco\0";

char   opsDisco[]       = "Velocidade Real\0"
                          "Velocidade Rápida\0";

// Prototipos
LRESULT CALLBACK WndProcInteiro(
	HWND   Handle,		// handle of window
	UINT   Mensagem,	// message identifier
	WPARAM wParam,		// first message parameter
	LPARAM lParam 		// second message parameter
);

LRESULT CALLBACK FrameWndProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam);
BOOL   CALLBACK ConfigDlgProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam);
BOOL CALLBACK ImportarDlgProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam);
BOOL CALLBACK ExportarDlgProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam);

void ProcessaBotao(int ID, HWND window);
void Drawframewindow();
void ResetMachineState();
void DrawStatusArea(HDC passdc, BOOL drawflags);
void DrawBitmapRect (HDC dc, int x, int y, LPRECT rect, HBITMAP bitmap);
void CreateGdiObjects(void);
void DeleteGdiObjects();
void SetUsingCursor (BOOL newvalue);
void EnableTrackbar (HWND window, BOOL enable);
void FillComboBox(HWND window, int controlid, LPCTSTR choices, int currentchoice);
void DrawCrosshairs (int x, int y);

// Funções
LRESULT CALLBACK WndProcInteiro(
	HWND   Handle,		// handle of window
	UINT   Mensagem,	// message identifier
	WPARAM wParam,		// first message parameter
	LPARAM lParam 		// second message parameter
)
{
	switch (Mensagem)
	{
		case WM_PAINT:
			//return 0;
		break;

		case WM_CLOSE:
			//
		break;

		case WM_CREATE:
			BotaoApertado = 0;
		break;

		case WM_DESTROY:
			//
		break;

		case WM_COMMAND:
		{
			if ((HWND)lParam == btnOK)
			{
				BotaoApertado = 1;
				return 0;
			}
			else if ((HWND)lParam == btnCancelar)
			{
				BotaoApertado = 2;
				return 0;
			}
		}
		break;
	}
	return DefWindowProc(Handle,Mensagem,wParam,lParam);
}

// Funções Callback
LRESULT CALLBACK FrameWndProc (HWND   window,
                               UINT   message,
                               WPARAM wparam,
                               LPARAM lparam)
{
	switch (message)
	{
		case WM_ACTIVATE:
			JoyReset();
			SetUsingCursor(0);
		break;

		case WM_ACTIVATEAPP:
			ativo = wparam;
		break;

		case WM_CLOSE:
			if (!restart && (mode != MODE_LOGO))
			{
				Som_Mute();
				if (MessageBox((HWND)framewindow,
							"Deseja realmente sair?",
							"Sair do Emulador",
							MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) == IDNO)
				{
					return 0;
				}
				Som_Demute();
			}
			if (!IsIconic(window))
				GetWindowRect(window,&framerect);
			RegSaveValue(PREFERENCIAS,POSX,1,framerect.left);
			RegSaveValue(PREFERENCIAS,POSY,1,framerect.top);
			FrameReleaseDC();
			SetUsingCursor(0);
			//ImageClose();
#ifdef USAR_HELP
			if (helpquit)
			{
				helpquit = 0;
				HtmlHelp(NULL,NULL,HH_CLOSE_ALL,0);
			}
#endif
		break;

		case WM_CHAR:
			if ((mode == MODE_RUNNING) || (mode == MODE_LOGO) ||
					((mode == MODE_STEPPING) && (wparam != '\x1B')))
				; //KeybQueueKeypress((int)wparam, 1);
			else if ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))
				DebugProcessChar((char)wparam);
		break;

		case WM_CREATE:
			framewindow = window;
			Inicializar2();
			CreateGdiObjects();
		break;

		case WM_DESTROY:
			DebugDestroy();
			Finalizar(restart);
			DeleteGdiObjects();
			PostQuitMessage(0);
		break;

		case WM_DISPLAYCHANGE:
			VideoReinitialize();
		break;

		case WM_KEYDOWN:
			if (((wparam >= VK_F1) && (wparam <= VK_F8))  &&
				(GetKeyState(VK_SHIFT) >= 0) &&
				(GetKeyState(VK_CONTROL) >= 0) &&
				(GetKeyState(VK_MENU) >= 0))
			{
				switch (wparam)
				{
					case VK_F1:
						ProcessaMenu(mnuAjuda, window);
					break;

					case VK_F2:
						ProcessaMenu(mnuDepurar, window);
					break;

					case VK_F3:
						ProcessaMenu(mnuConfig, window);
					break;

					case VK_F4:
						ProcessaMenu(mnuTapeInserir, window);
					break;

					case VK_F5:
						ProcessaMenu(mnuResetar, window);
					break;

					case VK_F6:
						ProcessaMenu(mnuDisco1, window);
					break;

					case VK_F7:
						ProcessaMenu(mnuDisco2, window);
					break;

					case VK_F8:
						//
					break;
				}
			}

			if ((wparam == VK_F9) &&
				(GetKeyState(VK_SHIFT) >= 0) &&
				(GetKeyState(VK_CONTROL) >= 0) &&
				(GetKeyState(VK_MENU) >= 0))
			{
				if (++videotype == VT_NUM_MODES)
					videotype = VT_MONO;
				VideoReinitialize();
				if ((mode != MODE_LOGO) && (mode != MODE_DEBUG))
					VideoRedrawScreen();
				RegSaveValue(CONFIG, EMUVIDEO, 1, videotype);
			}
			else if (wparam == VK_PAUSE)
			{
				SetUsingCursor(0);
				switch (mode)
				{
					case MODE_RUNNING:  mode = MODE_PAUSED;			break;
					case MODE_PAUSED:   mode = MODE_RUNNING;		break;
					case MODE_STEPPING: DebugProcessChar('\x1B');	break;
				}
				DrawStatusArea((HDC)0, DRAW_TITLE);
				if ((mode != MODE_LOGO) && (mode != MODE_DEBUG))
					VideoRedrawScreen();
				resettiming = 1;
			}
			else if ((mode == MODE_RUNNING) || (mode == MODE_LOGO) || (mode == MODE_STEPPING))
			{
				BOOL autorep  = ((lparam & 0x40000000) != 0);
				BOOL extended = ((lparam & 0x01000000) != 0);
				if ((!JoyProcessKey((int)wparam,extended,1,autorep)) &&
									(mode != MODE_LOGO))
				{
					KeybQueueKeypress((int)wparam, true, false);
				}
			}
			else if (mode == MODE_DEBUG)
			{
				DebugProcessCommand(wparam);
				return 0;
			}
		break;

		case WM_KEYUP:
			JoyProcessKey((int)wparam, ((lparam & 0x01000000) != 0), 0, 0);
			KeybQueueKeypress((int)wparam, false, false);
		break;


		case WM_LBUTTONDOWN:
		{
			int x = LOWORD(lparam);
			int y = HIWORD(lparam);
			int xx = 0, yy = VIDEOHEIGHT;
			RECT rect = {0,0,30,8};

			if (usingcursor)
				if (wparam & (MK_CONTROL | MK_SHIFT))
					SetUsingCursor(0);
				else
					JoySetButton(0,1);
				else if (JoyUsingMouse() &&
							((mode == MODE_RUNNING) || (mode == MODE_STEPPING)))
					SetUsingCursor(1);
		}
		break;

		case WM_LBUTTONUP:
			if (usingcursor)
				JoySetButton(0,0);
		break;

		case WM_MOUSEMOVE:
		{
			int x = LOWORD(lparam);
			int y = HIWORD(lparam);
			if (usingcursor)
			{
				DrawCrosshairs(x,y);
				JoySetPosition(x-VIDEOLEFT-2,VIDEOWIDTH-4,
								y-VIDEOTOP-2,VIDEOHEIGHT-4);
			}
		}
		break;

		case WM_PAINT:
			if (GetUpdateRect(window,NULL,0))
			{
				pintando = 1;
				Drawframewindow();
				pintando = 0;
			}
		break;

		case WM_PALETTECHANGED:
			if ((HWND)wparam == window)
				break;
      // fall through

		case WM_QUERYNEWPALETTE:
			Drawframewindow();
		break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if (usingcursor)
				JoySetButton(1,(message == WM_RBUTTONDOWN));
		break;

		case WM_SYSCOLORCHANGE:
			DeleteGdiObjects();
			CreateGdiObjects();
		break;

		case WM_SYSCOMMAND:

			switch (wparam & 0xFFF0)
			{
				case SC_KEYMENU:
					if (ativo)
						return 0;
				break;

				case SC_MINIMIZE:
					GetWindowRect(window,&framerect);
				break;

				case SC_MOUSEMENU:
					Som_Mute();
				break;
			}
		break;

		case WM_SYSKEYDOWN:
			PostMessage(window, WM_KEYDOWN, wparam, lparam);
			if ((wparam == VK_F10) || (wparam == VK_MENU))
				return 0;
		break;

		case WM_SYSKEYUP:
			PostMessage(window, WM_KEYUP, wparam, lparam);
		break;

		case WM_USER+2:
			if (mode != MODE_LOGO)
			{
				Som_Mute();
				if (MessageBox((HWND)framewindow,
							"Ao reiniciar o emulador o estado da máquina será "
							"resetado, causando perda de qualquer trabalho "
							"não salvo.\n\n"
							"Você quer continuar?",
							"Configuração",
							MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) == IDNO)
				{
					break;
				}
				Som_Demute();
			}
			restart = 1;
			PostMessage(window,WM_CLOSE,0,0);
		break;

		case WM_COMMAND:
		{
			if (HIWORD(wparam) == 0)
			{
				ProcessaMenu(LOWORD(wparam), window);
				return 0;
			}
		}
		break;

		case WM_SETFOCUS:
			KeybUnset();
			break;

		case WM_KILLFOCUS:
			KeybUnset();
			break;
	}
	return DefWindowProc(window,message,wparam,lparam);
}

//===========================================================================
BOOL CALLBACK ConfigDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam)
{
	static BOOL afterclose = 0;

	switch (message)
	{
		case WM_COMMAND:
			switch (LOWORD(wparam))
			{
				case IDOK:
				{
					DWORD newvidtype      = (DWORD)SendDlgItemMessage(window, cbVideo,       CB_GETCURSEL,0,0);
					DWORD newjoytype      = (DWORD)SendDlgItemMessage(window, cbJoystick,    CB_GETCURSEL,0,0);
					DWORD newsoundtype    = (DWORD)SendDlgItemMessage(window, cbSom,         CB_GETCURSEL,0,0);
					DWORD novoimpressora  = (DWORD)SendDlgItemMessage(window, cbImpressora,  CB_GETCURSEL,0,0);
					BOOL  newdisktype     = (BOOL) SendDlgItemMessage(window, cbDisco,       CB_GETCURSEL,0,0);

					NewSlotAux = (BYTE)SendDlgItemMessage(window, cbExpansao,    CB_GETCURSEL,0,0);
					// Se houve mudança no slot auxiliar
					if (NewSlotAux != SlotAux)
					{
						if (!MemAtualizaSlotAux())
						{
							afterclose = 0;
							return 0;
						}
					}

					if (novoimpressora != ImpressoraPorta) 
						ImpressoraDefinePorta(novoimpressora);

					if (newdisktype != enhancedisk)
					{
						if (MessageBox(window,
									"Você mudou a velocidade de acesso ao disco. "
									"Essa mudança terá efeito somente da próxima "
									"vez que o emulador for reiniciado.\n\n"
									"Você gostaria de reiniciar agora?",
									"Configuração",
									MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) == IDYES)
						{
							afterclose = 2;
						}
					}

					if (!JoySetEmulationType(window, newjoytype))
					{
						afterclose = 0;
						return 0;
					}
					if (!SpkrSetEmulationType(window, newsoundtype))
					{
						afterclose = 0;
						return 0;
					}
					// Scan Lines
					int NewScanLines = IsDlgButtonChecked(window, cbScanLines) ? 1 : 0;
					if (ScanLines != NewScanLines)
					{
						ScanLines = NewScanLines;
						VideoRedrawScreen();
					}

					ColarRapido = IsDlgButtonChecked(window, cbColarRapido) ? 1 : 0;

					if (videotype != newvidtype)
					{
						videotype = newvidtype;
						VideoReinitialize();
						if ((mode != MODE_LOGO) && (mode != MODE_DEBUG))
							VideoRedrawScreen();
					}
					if (IsDlgButtonChecked(window, rbVelReal))
						speed = SPEED_NORMAL;
					else
						speed = SendDlgItemMessage(window, slVel, TBM_GETPOS,0,0);
					SetCurrentCLK6502();

#define SAVE(a,b) RegSaveValue(CONFIG, a, 1, b);

					SAVE(EMUJOY     ,joytype);
					SAVE(EMUSOM     ,soundtype);
					SAVE(EMUSLOT    ,SlotAux);
					SAVE(EMUDISCO   ,newdisktype);
					SAVE(EMUVEL     ,IsDlgButtonChecked(window, rbOutraVel));
					SAVE(EMUVELCPU  ,speed);
					SAVE(EMUVIDEO   ,videotype);
					SAVE(EMUSCANLINES, ScanLines); // Scan Lines
					SAVE(EMUCOLARRAPIDO, ColarRapido);
#undef SAVE
					EndDialog(window,1);
					if (afterclose)
						PostMessage((HWND)framewindow, WM_USER+afterclose, 0, 0);
				break;
				}
				case IDCANCEL:
					EndDialog(window,0);
				break;

				case rbVelReal:
					SendDlgItemMessage(window, slVel, TBM_SETPOS, 1, SPEED_NORMAL);
					EnableTrackbar(window,0);
				break;

				case rbOutraVel:
					SetFocus(GetDlgItem(window,slVel));
					EnableTrackbar(window,1);
				break;

				case slVel:
					CheckRadioButton(window,rbVelReal,rbOutraVel,rbOutraVel);
					EnableTrackbar(window,1);
				break;
			}
			break;

		case WM_HSCROLL:
			CheckRadioButton(window,rbVelReal, rbOutraVel, rbOutraVel);
		break;

		case WM_INITDIALOG:
			FillComboBox(window, cbVideo,      opsVideo,     videotype);
			FillComboBox(window, cbJoystick,   opsJoystick,  joytype);
			FillComboBox(window, cbImpressora, opsImpressora,ImpressoraPorta);
			FillComboBox(window, cbSom,        opsSom,       soundtype);
			FillComboBox(window, cbExpansao,   opsExpansao,  SlotAux);
			FillComboBox(window, cbDisco,      opsDisco,     enhancedisk);
			SendDlgItemMessage(window, slVel, TBM_SETRANGE,1,MAKELONG(0,40));
			SendDlgItemMessage(window, slVel, TBM_SETPAGESIZE,0,5);
			SendDlgItemMessage(window, slVel, TBM_SETTICFREQ,10,0);
			SendDlgItemMessage(window, slVel, TBM_SETPOS,1,speed);
			{
				BOOL custom = 1;
				if (speed == 10)
				{
					custom = 0;
					RegLoadValue(CONFIG, EMUVEL, 1,(DWORD *)&custom);
				}
				CheckRadioButton(window, rbVelReal, rbOutraVel, rbVelReal + custom);
				SetFocus(GetDlgItem(window, custom ? slVel : rbVelReal));
				EnableTrackbar(window,custom);
			}
			CheckDlgButton(window, cbScanLines,   ScanLines   ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(window, cbColarRapido, ColarRapido ? BST_CHECKED : BST_UNCHECKED);
			afterclose = 0;
		break;
	}
	return 0;
}

//===========================================================================
BOOL CALLBACK ImportarDlgProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam)
{
	switch (message)
	{
		case WM_COMMAND:
			switch (LOWORD(wparam))
			{
				int  EndInicial;
				char EndI[MAX_PATH];

				case IDOK:
					GetDlgItemText(window, edtImpEndInicial, EndI, MAX_PATH);
					if (EndI[0] == '$')
					{
						char temp[MAX_PATH];

						strcpy(temp,"0x");
						strcat(temp,&EndI[1]);
						sscanf(temp,"%x",&EndInicial);
					}
					else if (!strncmp(EndI,"0x",2))
					{
						sscanf(EndI,"%x",&EndInicial);
					}
					else
						EndInicial = atoi(EndI);
					EndInicial &= 0xFFFF;
					if (EndInicial < 0xBFFF && strlen(EndI))
					{
						if (!MemImportar(EndInicial))
						{
							RegSaveString(PREFERENCIAS, ULTIMPEND, 1, EndI);
							EndDialog(window,1);
						}
					}
					else
					{
						MessageBox(window,
									"Digite os valores corretamente",
									"Erro",
									MB_OK | MB_ICONWARNING);
					}
				break;

				case IDCANCEL:
					EndDialog(window,0);
				break;
			} // switch loword
		break;

		case WM_INITDIALOG:
			{
				char Endereco[MAX_PATH] = "";

				RegLoadString(PREFERENCIAS, ULTIMPEND, 1, Endereco, MAX_PATH);
				SetDlgItemText(window, edtImpEndInicial, Endereco);
				SetFocus(GetDlgItem(window, edtImpEndInicial));
			}
		break;
	} // switch message
	return 0;
}

//===========================================================================
BOOL CALLBACK ExportarDlgProc(  HWND   window,
								UINT   message,
								WPARAM wparam,
								LPARAM lparam)
{
	switch (message)
	{
		case WM_COMMAND:
			switch (LOWORD(wparam))
			{
				int  EndInicial;
				int  EndFinal;
				char EndI[MAX_PATH];
				char EndF[MAX_PATH];

				case IDOK:
					GetDlgItemText(window, edtExpEndInicial, EndI, MAX_PATH);
					GetDlgItemText(window, edtExpEndFinal,   EndF, MAX_PATH);
					if (EndI[0] == '$')
					{
						char temp[MAX_PATH];
						strcpy(temp,"0x");
						strcat(temp,&EndI[1]);
						sscanf(temp,"%x",&EndInicial);
					}
					else if (!strncmp(EndI,"0x",2))
					{
						sscanf(EndI,"%x",&EndInicial);
					}
					else
						EndInicial = atoi(EndI);
					if (EndF[0] == '$')
					{
						char temp[MAX_PATH];
						strcpy(temp,"0x");
						strcat(temp,&EndF[1]);
						sscanf(temp,"%x",&EndFinal);
					}
					else if (!strncmp(EndF,"0x",2))
					{
						sscanf(EndF,"%x",&EndFinal);
					}
					else
						EndFinal = atoi(EndF);
					EndInicial &= 0xFFFF;
					EndFinal &= 0xFFFF;
					if (EndInicial < 0xC000 && EndFinal < 0xC000 &&
						(EndInicial || EndFinal) &&
						(EndInicial < EndFinal))
					{
						if (!MemExportar(EndInicial, EndFinal))
						{
							RegSaveString(PREFERENCIAS, ULTEXPENDI, 1, EndI);
							RegSaveString(PREFERENCIAS, ULTEXPENDF, 1, EndF);
							EndDialog(window,1);
						}
					}
					else
					{
						MessageBox(window,
									"Digite os valores corretos",
									"Erro",
									MB_OK | MB_ICONWARNING);
					}
				break;

				case IDCANCEL:
					EndDialog(window,0);
				break;
			} // switch loword
		break;

		case WM_INITDIALOG:
			{
				char Endereco[MAX_PATH] = "";

				RegLoadString(PREFERENCIAS, ULTEXPENDI, 1, Endereco, MAX_PATH);
				SetDlgItemText(window, edtExpEndInicial, Endereco);
				RegLoadString(PREFERENCIAS, ULTEXPENDF, 1, Endereco, MAX_PATH);
				SetDlgItemText(window, edtExpEndFinal, Endereco);
				SetFocus(GetDlgItem(window, edtExpEndInicial));
			}
		break;
	} // switch message
	return 0;
}

// Funções Internas
//===========================================================================
void ProcessaMenu(int ID, HWND window) {
	switch(ID) {
		case mnuSair:
			SendMessage(window, WM_CLOSE, 0, 0);
			break;

		case mnuDepurar:
			if (mode == MODE_LOGO)
				ResetMachineState();
			if (mode == MODE_STEPPING)
				DebugProcessChar('\x1B');
			else if (mode == MODE_DEBUG)
				ProcessaMenu(mnuResetar, window);
			else {
				KeybUnset();
				DebugBegin();
			}
			DrawStatusArea((HDC)0,DRAW_TITLE);
			return;
			break;

		case mnuConfig:
			DialogBox(instance,
						"DIALOGO_CONFIG",
						framewindow,
						(DLGPROC)ConfigDlgProc);
			break;

		case mnuSobre:
			MessageBox(window,
						"Emulador TK2000 para windows, baseado "
						"no emulador Applewin por Michael O'Brien.\n\n"
						"Parte do código foi usado do emulador Applewin do Tom Charlesworth.\n\n"
						"Adaptado por Fábio Belavenuto, Copyright 2004.",
						"Sobre o TK2000",
						MB_OK | MB_ICONINFORMATION
						);
			break;

		case mnuResetar:
			if (mode == MODE_LOGO)
				DiskBoot();
			else if (mode == MODE_RUNNING)
				ResetMachineState();
			if ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))
				DebugEnd();
			mode = MODE_RUNNING;
			DrawStatusArea((HDC)0,DRAW_TITLE);
			VideoRedrawScreen();
			resettiming = 1;
			break;

		case mnuCor:
			VideoChooseColor();
			break;

		case mnuImportar:
			DialogBox(instance,
						"DIALOGO_IMPORTAR",
						framewindow,
						(DLGPROC)ImportarDlgProc);
		
			break;

		case mnuExportar:
			DialogBox(instance,
						"DIALOGO_EXPORTAR",
						framewindow,
						(DLGPROC)ExportarDlgProc);
			break;

		case mnuDisco1:
			DiskSelect(0);
			DrawStatusArea(0, DRAW_BACKGROUND | DRAW_LEDS);
			break;

		case mnuRemDisco1:
			DiskRemove(0);
			DrawStatusArea(0, DRAW_BACKGROUND | DRAW_LEDS);
			break;

		case mnuVolDisco1:
			DiskEscolheVolume(0);
			break;

		case mnuDisco2:
			DiskSelect(1);
			DrawStatusArea(0, DRAW_BACKGROUND | DRAW_LEDS);
			break;

		case mnuRemDisco2:
			DiskRemove(1);
			DrawStatusArea(0, DRAW_BACKGROUND | DRAW_LEDS);
			break;

		case mnuVolDisco2:
			DiskEscolheVolume(1);
			break;

		case mnuTapeInserir:
			TapeSelect();
			break;

		case mnuTapeRemove:
			TapeRemove();
			break;

		case mnuTapeRew:
			TapeBotao(TB_REW);
			break;

		case mnuTapeStop:
			TapeBotao(TB_STOP);
			break;

		case mnuExtrairBasic:
			if (mode == MODE_RUNNING) {
				char *applesoft = UtilExtrairApplesoft();
				if (!applesoft) {
					FrameMostraMensagemErro("Erro ao extrair!");
				} else {
					if (!OpenClipboard((HWND)framewindow)) {
						FrameMostraMensagemErro("Erro ao abrir a Área de Transferência");
						free(applesoft);
					} else {
						EmptyClipboard();
						HGLOBAL hText = GlobalAlloc(GMEM_DDESHARE, strlen(applesoft)+1);
						if (hText == NULL) {
							FrameMostraMensagemErro("Erro ao abrir a Área de Transferência");
							CloseClipboard();
							free(applesoft);
						} else {
							char *pText = (char *)GlobalLock(hText);
							strcpy(pText, LPCSTR(applesoft));
							GlobalUnlock(hText); 
							SetClipboardData(CF_TEXT, hText);
							CloseClipboard();
							free(applesoft);
							FrameMostraMensagemAdvertencia("Copiado para área de transferência!");
						}
					}
				}
			} else {
				FrameMostraMensagemErro("Somente no modo de execução");
			}

			break;
	}
	//Som_Demute();
}

//===========================================================================
void HabilitaMenu(UINT ID)
{
	HMENU Menu;
	MENUITEMINFO item;

	Menu = GetMenu((HWND)framewindow);
	Menu = GetSubMenu(Menu, 1);
	ZeroMemory(&item, sizeof(item));
	item.cbSize     = sizeof(item);
	item.fMask      = MIIM_ID | MIIM_STATE;
	item.fState		= MFS_ENABLED;
	item.wID        = ID;
	SetMenuItemInfo(Menu, ID, FALSE, &item);
}

//===========================================================================
void DesabilitaMenu(UINT ID)
{
	HMENU Menu;
	MENUITEMINFO item;

	Menu = GetMenu((HWND)framewindow);
	Menu = GetSubMenu(Menu, 1);
	ZeroMemory(&item, sizeof(item));
	item.cbSize     = sizeof(item);
	item.fMask      = MIIM_ID | MIIM_STATE;
	item.fState		= MFS_GRAYED;
	item.wID        = ID;
	SetMenuItemInfo(Menu, ID, FALSE, &item);
}

//===========================================================================
void CreateGdiObjects(void)
{
#define LOADBUTTONBITMAP(bitmapname)  LoadImage(instance,bitmapname,   \
                                                IMAGE_BITMAP,0,0,      \
                                                LR_CREATEDIBSECTION |  \
                                                LR_LOADMAP3DCOLORS |   \
                                                LR_LOADTRANSPARENT);
	leds[0] = (HBITMAP)LOADBUTTONBITMAP("LED_OFF");
	leds[1] = (HBITMAP)LOADBUTTONBITMAP("LED_VD");
	leds[2] = (HBITMAP)LOADBUTTONBITMAP("LED_VM");
	btnfacebrush    = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	btnfacepen      = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
	btnhighlightpen = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNHIGHLIGHT));
	btnshadowpen    = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
	smallfont = CreateFont(11,6,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
							OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,VARIABLE_PITCH | FF_SWISS,
							"Small Fonts");
}

//===========================================================================
void DeleteGdiObjects() {
	DeleteObject(btnfacebrush);
	DeleteObject(btnfacepen);
	DeleteObject(btnhighlightpen);
	DeleteObject(btnshadowpen);
	DeleteObject(smallfont);
}

//===========================================================================
void ResetMachineState() {
	MemReset();
	VideoResetState();
	JoyReset();
}

//===========================================================================
void DrawBitmapRect (HDC dc, int x, int y, LPRECT rect, HBITMAP bitmap) {
	HDC memdc = CreateCompatibleDC(dc);
	SelectObject(memdc,bitmap);
	BitBlt(dc,x,y,
			rect->right  + 1 - rect->left,
			rect->bottom + 1 - rect->top,
			memdc,
			rect->left,
			rect->top,
			SRCCOPY);
	DeleteDC(memdc);
}

//===========================================================================
void DrawStatusArea(HDC passdc, int drawflags) {
	HDC  dc;
	int  x;
	int  y;
	int  drive1;
	int  drive2;
	int  TapeLed;
//	int  HD1;
//	int  HD2;
//	int  IDELed;

	FrameReleaseDC();
	dc     = (passdc ? passdc : GetDC(framewindow));
	x      = 0;
	y      = VIDEOHEIGHT;
	DiskGetLightStatus(&drive1,&drive2);
	TapePegaStatusLed(&TapeLed);

	if (drawflags & DRAW_BACKGROUND) {
		char temp[MAX_PATH];

		SelectObject(dc, GetStockObject(NULL_PEN));
		SelectObject(dc, btnfacebrush);
		Rectangle(dc, x, y, x+VIDEOWIDTH+2, y+ STATUSHEIGHT+2);
		SelectObject(dc, smallfont);
		SetTextAlign(dc, TA_TOP);
		SetTextColor(dc, RGB(0,0,0));
		SetBkMode(dc, TRANSPARENT);
		strcpy(temp, "Drive 1 - ");
		strcat(temp, DiskGetName(0));
		TextOut(dc,
				x + STATUSXPOS1+10,
				y + STATUSYPOS1-1,
				temp,
				strlen(temp));
		strcpy(temp, "Drive 2 - ");
		strcat(temp, DiskGetName(1));
		TextOut(dc,
			    x + STATUSXPOS1+10,
				y + STATUSYPOS2-1,
				temp,
				strlen(temp));
		strcpy(temp, "Tape - ");
		strcat(temp, TapeNomeImagem());
		TextOut(dc,
				x + STATUSXPOS2+10,
				y + STATUSYPOS1-1,
				temp,
				strlen(temp));
	}
	if (drawflags & DRAW_LEDS) {
		RECT rect = {0,0,8,8};
		DrawBitmapRect(dc, x + STATUSXPOS1, y + STATUSYPOS1, &rect, leds[drive1]);
		DrawBitmapRect(dc, x + STATUSXPOS1, y + STATUSYPOS2, &rect, leds[drive2]);
		DrawBitmapRect(dc, x + STATUSXPOS2, y + STATUSYPOS1, &rect, leds[TapeLed]);
	}
	if (drawflags & DRAW_TITLE) {
		char title[40];
		strcpy(title,TITULO);
		switch (mode) {
			case MODE_PAUSED:
				Som_Mute();
				strcat(title," [Pausado]");
			break;

			case MODE_RUNNING:
				strcat(title," [Rodando]");
			break;

			case MODE_DEBUG:
				Som_Mute();
				strcat(title," [Depurando]");
			break;

			case MODE_STEPPING:
				Som_Mute();
				strcat(title," [Passo à Passo]");
			break;
		}
		SendMessage((HWND)framewindow,WM_SETTEXT,0,(LPARAM)title);
	}
	if (!passdc)
		ReleaseDC((HWND)framewindow,dc);
}

//===========================================================================
void Drawframewindow(void) {
	PAINTSTRUCT ps;
	HDC dc;

	FrameReleaseDC();
	dc = (pintando ? BeginPaint((HWND)framewindow,&ps)
					: GetDC((HWND)framewindow));
	VideoRealizePalette(dc);
	DrawStatusArea(dc,DRAW_BACKGROUND | DRAW_LEDS);

	if (pintando)
		EndPaint((HWND)framewindow,&ps);
	else
		ReleaseDC((HWND)framewindow,dc);

	// DRAW THE CONTENTS OF THE EMULATED SCREEN
	if (mode == MODE_LOGO)
		VideoDisplayLogo();
	else if (mode == MODE_DEBUG)
		DebugDisplay(1);
	else
		VideoRedrawScreen();
}

//===========================================================================
void DrawCrosshairs(int x, int y) {
	//
}

//===========================================================================
void EnableTrackbar (HWND window, BOOL enable) {
	int loop = slVel;

	EnableWindow(GetDlgItem(window,slVel), enable);
	while (loop++ < 124)
		EnableWindow(GetDlgItem(window,loop),enable);
}

//===========================================================================
void SetUsingCursor (BOOL newvalue) {
	static HCURSOR cursorvelho;

	if (newvalue == usingcursor)
		return;
	usingcursor = newvalue;
	if (usingcursor) {
		RECT rect;
		POINT pt;

		SetCapture((HWND)framewindow);
		rect.left   = VIDEOLEFT;
		rect.top    = VIDEOTOP;
		rect.right  = VIDEOLEFT+VIDEOWIDTH;
		rect.bottom = VIDEOTOP+VIDEOHEIGHT;
		ClientToScreen((HWND)framewindow,(LPPOINT)&rect.left);
		ClientToScreen((HWND)framewindow,(LPPOINT)&rect.right);
		ClipCursor(&rect);
//		ShowCursor(0);
		cursorvelho = SetCursor(LoadCursor(0,IDC_CROSS));
		GetCursorPos(&pt);
		ScreenToClient((HWND)framewindow,&pt);
//		DrawCrosshairs(pt.x,pt.y);
	} else {
		DrawCrosshairs(0,0);
//		ShowCursor(1);
		SetCursor(cursorvelho);
		ClipCursor(NULL);
		ReleaseCapture();
	}
}

//===========================================================================
void FillComboBox(HWND window, int controlid, LPCTSTR choices, int currentchoice) {
	HWND combowindow = GetDlgItem(window,controlid);
	SendMessage(combowindow,CB_RESETCONTENT,0,0);
	while (*choices) {
		SendMessage(combowindow,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)choices);
		choices += strlen(choices)+1;
	}
	SendMessage(combowindow,CB_SETCURSEL,currentchoice,0);
}


//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
void FrameCreateWindow() {
	int xpos, ypos, width, height;

	width  = VIDEOWIDTH  +  (GetSystemMetrics(SM_CXBORDER)<<1) + 4;
	height = VIDEOHEIGHT +  (GetSystemMetrics(SM_CYBORDER)<<1) +
							GetSystemMetrics(SM_CYCAPTION) +
							GetSystemMetrics(SM_CYMENU) + 
							STATUSHEIGHT + 4;

	if (!RegLoadValue(PREFERENCIAS,POSX,1,(DWORD *)&xpos))
		xpos = (GetSystemMetrics(SM_CXSCREEN)-width) >> 1;
	if (!RegLoadValue(PREFERENCIAS,POSY,1,(DWORD *)&ypos))
		ypos = (GetSystemMetrics(SM_CYSCREEN)-height) >> 1;

	framewindow = CreateWindow("TK2000FRAME",
								TITULO,
								WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
								WS_MINIMIZEBOX | WS_VISIBLE,
								xpos,
								ypos,
								width,
								height,
								HWND_DESKTOP,
								(HMENU)LoadMenu(instance, "MENU"),
								instance,
								NULL);
}

//================================================================================
int FrameGetDC() {
  if (!framedc)  {
    framedc = GetDC((HWND)framewindow);
//    SetViewportOrgEx(framedc,viewportx,viewporty,NULL);
  }
  return (int)framedc;
}

//================================================================================
int FrameGetVideoDC(char* *addr, LONG *pitch) {
	return FrameGetDC();
}

//================================================================================
void FrameRefreshStatus(int drawflags) {
	DrawStatusArea((HDC)0,drawflags);
}

//================================================================================
void FrameRegisterClass() {
  WNDCLASSEX wndclass;

  ZeroMemory(&wndclass,sizeof(WNDCLASSEX));
  wndclass.cbSize        = sizeof(WNDCLASSEX);
  wndclass.style         = CS_OWNDC | CS_BYTEALIGNCLIENT;
  wndclass.lpfnWndProc   = FrameWndProc;
  wndclass.hInstance     = instance;
  wndclass.hIcon         = LoadIcon(instance,"ICONE_TK2000");
  wndclass.hIconSm       = (HICON)LoadImage(instance,"ICONE_TK2000",
                                            IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
  wndclass.hCursor       = LoadCursor(0,IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wndclass.lpszClassName = "TK2000FRAME";
  wndclass.lpszMenuName  = NULL;
  RegisterClassEx(&wndclass);
}

//================================================================================
void FrameReleaseDC() {
  if (framedc) {
//    SetViewportOrgEx(framedc,0,0,NULL);
    ReleaseDC((HWND)framewindow,framedc);
    framedc = (HDC)0;
  }
}

//================================================================================
void FrameReleaseVideoDC() {
/*
  if (active && !pintando)
  {
    // THIS IS CORRECT ACCORDING TO THE DIRECTDRAW DOCS
    RECT rect = {FSVIEWPORTX,
                 FSVIEWPORTY,
                 FSVIEWPORTX+VIEWPORTCX,
                 FSVIEWPORTY+VIEWPORTCY};
    surface->Unlock(&rect);

    // BUT THIS SEEMS TO BE WORKING
    surface->Unlock(NULL);
  }
*/
}

//================================================================================
void FrameMenuDiskete(int Habilitar) {
	if (Habilitar) {
		HabilitaMenu(mnuDisco1);
		HabilitaMenu(mnuDisco2);
		HabilitaMenu(mnuRemDisco1);
		HabilitaMenu(mnuRemDisco2);
		HabilitaMenu(mnuVolDisco1);
		HabilitaMenu(mnuVolDisco2);
	} else {
		DesabilitaMenu(mnuDisco1);
		DesabilitaMenu(mnuDisco2);
		DesabilitaMenu(mnuRemDisco1);
		DesabilitaMenu(mnuRemDisco2);
		DesabilitaMenu(mnuVolDisco1);
		DesabilitaMenu(mnuVolDisco2);
	}
}

//================================================================================
void FrameMostraMensagemAdvertencia(char *Mensagem) {
	Som_Mute();
	MessageBox((HWND)framewindow,
				Mensagem,
				TITULO,
				MB_ICONEXCLAMATION | MB_SETFOREGROUND);
	Som_Demute();
}

//================================================================================
void FrameMostraMensagemErro(char *Mensagem) {
	Som_Mute();
	MessageBox((HWND)framewindow,
				Mensagem,
				TITULO,
				MB_ICONSTOP | MB_SETFOREGROUND);
	Som_Demute();
}

/*
 *  Funções Auxiliares
 *  Versão 0.1
 *  by Fábio Belavenuto
 */
//================================================================================
int FramePerguntaInteiro(char *Titulo, int Default) {
	int			X, Y, result;
	WNDCLASSEX	wndclass;
	MSG			Mensagem;
	char		temp[MAX_PATH];

	X = (GetSystemMetrics(SM_CXSCREEN) >> 1) - (WIDTH_DIALOGO  >> 1);
	Y = (GetSystemMetrics(SM_CYSCREEN) >> 1) - (HEIGHT_DIALOGO >> 1);

	ZeroMemory(&wndclass,sizeof(WNDCLASSEX));
	wndclass.cbSize        = sizeof(WNDCLASSEX);
	wndclass.style         = CS_OWNDC | CS_BYTEALIGNCLIENT;
	wndclass.lpfnWndProc   = WndProcInteiro;
	wndclass.hInstance     = instance;
	wndclass.hIcon         = LoadIcon  (0,IDI_QUESTION);
	wndclass.hIconSm       = NULL; //LoadIcon  (0,IDI_QUESTION);
	wndclass.hCursor       = LoadCursor(0,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wndclass.lpszClassName = "DialogoInteiro";
	wndclass.lpszMenuName  = NULL;
	RegisterClassEx(&wndclass);

	Janela = CreateWindow(  "DialogoInteiro",
							Titulo,
							WS_DLGFRAME	| WS_VISIBLE | WS_SYSMENU,
							X,
							Y,
							WIDTH_DIALOGO,
							HEIGHT_DIALOGO,
							(HWND)framewindow,
							NULL,
							instance,
							NULL);

	Rotulo = CreateWindow(  "STATIC",
							"Digite um valor inteiro:",
							WS_CHILD | WS_VISIBLE,
							POSX_ROTULO,
							POSY_ROTULO,
							200,
							16,
							Janela,
							NULL,
							instance,
							NULL);

	edtValor = CreateWindowEx( WS_EX_CLIENTEDGE,
							"EDIT",
							"",
							WS_CHILD | WS_VISIBLE | WS_BORDER |
							WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
							POSX_CEDIT,
							POSY_CEDIT,
							55,
							24,
							Janela,
							NULL,
							instance,
							NULL);

	btnOK = CreateWindowEx( 0,
							"BUTTON",
							"OK",
							WS_CHILD | WS_VISIBLE |
							WS_TABSTOP | BS_DEFPUSHBUTTON,
							POSX_BTNOK,
							POSY_BTNOK,
							80,
							25,
							Janela,
							NULL,
							instance,
							NULL);

	btnCancelar = CreateWindowEx( 0,
							"BUTTON",
							"CANCELAR",
							WS_CHILD | WS_VISIBLE |
							WS_TABSTOP,
							POSX_BTNCANC,
							POSY_BTNCANC,
							80,
							25,
							Janela,
							NULL,
							instance,
							NULL);

	sprintf(temp,"%d",Default);
	SetWindowText(edtValor,temp);
	while (1) {
		if (BotaoApertado == 2) {
			DestroyWindow(Janela);
			UnregisterClass("DialogoInteiro", instance);
			return Default;
		}
		if (BotaoApertado == 1) {
			GetWindowText(edtValor, temp, MAX_PATH);
			result = atoi(temp);
			DestroyWindow(Janela);
			UnregisterClass("DialogoInteiro", instance);
			return result;
		}
		if (PeekMessage(&Mensagem,0,0,0,PM_REMOVE)) {
			if (Mensagem.message == WM_QUIT) {
				UnregisterClass("DialogoInteiro", instance);
				return Default;
			}
			TranslateMessage(&Mensagem);
			DispatchMessage(&Mensagem);
		}
	}
	UnregisterClass("DialogoInteiro", instance);
	return Default;
}

// EOF