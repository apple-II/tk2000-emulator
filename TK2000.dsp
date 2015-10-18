# Microsoft Developer Studio Project File - Name="TK2000" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TK2000 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TK2000.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TK2000.mak" CFG="TK2000 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TK2000 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TK2000 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TK2000 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "c:\dxsdk\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x416 /d "NDEBUG"
# ADD RSC /l 0x416 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib ddraw.lib gdi32.lib kernel32.lib shell32.lib user32.lib winmm.lib dsound.lib ole32.lib dxguid.lib version.lib strmiids.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "TK2000 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "C:\DXSDK\Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x416 /d "_DEBUG"
# ADD RSC /l 0x416 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib ddraw.lib gdi32.lib kernel32.lib shell32.lib user32.lib winmm.lib dsound.lib ole32.lib dxguid.lib version.lib strmiids.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TK2000 - Win32 Release"
# Name "TK2000 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\tk_cpu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_debug.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_disco.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_imagem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_impressora.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_ioapi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_janela.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_joystick.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_main.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_memoria.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_registro.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_som.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_som_nucleo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_tape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_teclado.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_timer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_unzip.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_video.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tk_zip.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\tk_cpu.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_debug.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_disco.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_imagem.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_impressora.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_janela.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_joystick.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_main.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_memoria.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_registro.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_som.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_som_nucleo.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_stdhdr.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_tape.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_teclado.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_timer.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_unzip.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_video.h
# End Source File
# Begin Source File

SOURCE=.\src\tk_zip.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\recursos\Charset.bmp
# End Source File
# Begin Source File

SOURCE=.\recursos\Disco.ico
# End Source File
# Begin Source File

SOURCE=.\recursos\disk.ico
# End Source File
# Begin Source File

SOURCE=.\recursos\DiskWrite.bmp
# End Source File
# Begin Source File

SOURCE=.\recursos\LedGreen.bmp
# End Source File
# Begin Source File

SOURCE=.\recursos\LedOff.bmp
# End Source File
# Begin Source File

SOURCE=.\recursos\LedRed.bmp
# End Source File
# Begin Source File

SOURCE=.\recursos\TK2000.ico
# End Source File
# Begin Source File

SOURCE=.\recursos\TK2000.rc
# End Source File
# End Group
# Begin Group "Textos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\COPYING.txt
# End Source File
# Begin Source File

SOURCE=.\Leiame.txt
# End Source File
# Begin Source File

SOURCE=.\Licenca.txt
# End Source File
# Begin Source File

SOURCE=.\Mudancas.txt
# End Source File
# Begin Source File

SOURCE=.\Tarefas.txt
# End Source File
# End Group
# End Target
# End Project
