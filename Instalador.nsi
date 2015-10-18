#
# Script de Geração de Instalação
# by Fábio Belavenuto
#

# Definições:
#!define USAR_HELP
!define USAR_LICENCA

!define NOME_PACOTE	"TK2000"
!define VERSAO		"0.0.0.1"
!define NOMESAIDA	"${NOME_PACOTE}-${VERSAO}-Instalador.exe"
!define NOME_PRODUTO	"Emulador TK2000 para Windows versão ${VERSAO}"

!define TK2000_EXE	"${NOME_PACOTE}.exe"
!define TK2000_SYM	"${NOME_PACOTE}.sym"
!define TK2000_BMP	"${NOME_PACOTE}.bmp"
!ifdef USAR_HELP
!define TK2000_HELP	"${NOME_PACOTE}.hlp"
!endif

!define TK2000_ROM	"${NOME_PACOTE}.rom"
!define TK2000_ROMDISCO	"Firmware_Disco.rom"

!define TK2000_DISCO_TKDOS	"TKDOS2000.dsk"

!ifdef USAR_LICENCA
!define ARQUIVO_LICENCA	"Licenca.txt"
!endif

!ifndef NSIS_DIR
!define NSIS_DIR      "C:\Arquiv~1\NSIS"
!endif

# Compressor padrão:
SetCompressor bzip2

# Incluir Interface Moderna:
!include "MUI.nsh"

# Definições Gerais:
Name "${NOME_PRODUTO}"
OutFile "${NOMESAIDA}"
InstallDir "$PROGRAMFILES\${NOME_PACOTE}"
InstallDirRegKey HKEY_LOCAL_MACHINE "Software\${NOMEPACOTE}\${NOMEPACOTE}" "instpath"
SetOverwrite on
ShowInstDetails show
XPStyle On

# Configurações da Interface:
!define  MUI_ABORTWARNING
!define  MUI_WELCOMEPAGE_TITLE_3LINES
!define  MUI_WELCOMEPAGE_TEXT "\r\n\r\nIsto instalará o ${NOME_PRODUTO} no seu computador.\r\n\r\n\r\n\r\n"
!define  MUI_COMPONENTSPAGE_NODESC
!define  MUI_FINISHPAGE_NOREBOOTSUPPORT

# Páginas:
!insertmacro MUI_PAGE_WELCOME
!ifdef USAR_LICENCA
!insertmacro MUI_PAGE_LICENSE ${ARQUIVO_LICENCA}
!endif
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
  
# Linguas:
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "English"

# Tipos Instalação:
InstType "Completo (Emulador, ROMs e Disketes)"
InstType "Normal (Somente Emulador e ROMs)"
InstType "Pequeno (Somente Emulador)"


# Seções:
Section "${NOME_PRODUTO}"
	SectionIn 1 2 3

	SetOutPath $INSTDIR
	File Leiame.txt
	File release\${TK2000_EXE}
	File outros\${TK2000_SYM}
	File outros\${TK2000_BMP}
	!ifdef USAR_HELP
	File help\${TK2000_HELP}
	!endif
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NOME_PACOTE}" "DisplayName" "${NOME_PRODUTO}"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NOME_PACOTE}" "UninstallString" '"$INSTDIR\un${TK2000_EXE}"'
	WriteUninstaller "un${TK2000_EXE}"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\${NOME_PACOTE}\${NOME_PACOTE}" "instpath" $INSTDIR
	WriteRegStr HKEY_CURRENT_USER "Software\${NOME_PACOTE}\${NOME_PACOTE}" "instpath" $INSTDIR
	
SectionEnd

#------------------------------------------------------------------------------------------
Section "Imagem de ROMs"
	SectionIn 1 2

	SetOutPath $INSTDIR
	File outros\${TK2000_ROM}
	File outros\${TK2000_ROMDISCO}
	
SectionEnd

#------------------------------------------------------------------------------------------
Section "Imagem de Disketes"
	SectionIn 1

	SetOutPath $INSTDIR
	File outros\${TK2000_DISCO_TKDOS}
SectionEnd

#------------------------------------------------------------------------------------------
Section "Adiciona ${NOME_PACOTE} no menu iniciar"
	SectionIn 1 2 3
	CreateDirectory "$SMPROGRAMS\${NOME_PACOTE}"

	CreateShortCut "$SMPROGRAMS\${NOME_PACOTE}\${NOME_PACOTE}.lnk" "$INSTDIR\${TK2000_EXE}" "" "$INSTDIR\${TK2000_EXE}" 0
	CreateShortCut "$SMPROGRAMS\${NOME_PACOTE}\Desinstalar ${NOME_PACOTE}.lnk" "$INSTDIR\un${TK2000_EXE}"
SectionEnd

# Funções:
Function .onInstSuccess

  MessageBox MB_YESNO|MB_ICONQUESTION \
             "Você gostaria de iniciar o programa agora?" \
             IDNO NaoExecuta
    Exec "$INSTDIR\${TK2000_EXE}"
  NaoExecuta:
FunctionEnd

Function .onInit
  ;
FunctionEnd


#------------------------------------------------------------------------------------------
Section -PostInstall
  ;
SectionEnd

Section Uninstall

	DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NOME_PACOTE}" "UninstallString"
	DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NOME_PACOTE}" "DisplayName"
	DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NOME_PACOTE}"

	DeleteRegKey HKEY_LOCAL_MACHINE "Software\${NOME_PACOTE}\${NOME_PACOTE}"

	DeleteRegKey HKEY_CURRENT_USER "Software\${NOME_PACOTE}\${NOME_PACOTE}"

	Delete "$INSTDIR\${TK2000_EXE}"
	Delete "$INSTDIR\${TK2000_SYM}"
	Delete "$INSTDIR\${TK2000_BMP}"
	Delete "$INSTDIR\Leiame.txt"
	!ifdef USAR_HELP"
	Delete "$INSTDIR\${TK2000_HELP}"
	!endif
	!ifdef USAR_LICENCA"
	Delete "$INSTDIR\${ARQUIVO_LICENCA}"
	!endif
	Delete "$INSTDIR\${TK2000_ROM}"
	Delete "$INSTDIR\${TK2000_ROMDISCO}"
	Delete "$INSTDIR\${TK2000_DISCO_TKDOS}"
	Delete "$SMPROGRAMS\${NOME_PACOTE}\Desinstalar ${NOME_PACOTE}.lnk"
	Delete "$SMPROGRAMS\${NOME_PACOTE}\${NOME_PACOTE}.lnk"
	RMDir  "$SMPROGRAMS\${NOME_PACOTE}"
	Delete "$INSTDIR\un${TK2000_EXE}"
	RMDir  "$INSTDIR"

SectionEnd
