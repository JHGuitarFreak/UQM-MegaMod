# Microsoft Developer Studio Project File - Name="UrQuanMasters" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=UrQuanMasters - Win32 Debug NoAccel
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UrQuanMasters.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UrQuanMasters.mak" CFG="UrQuanMasters - Win32 Debug NoAccel"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UrQuanMasters - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Debug NoAccel" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Release NoAccel" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W2 /GX /Zi /O2 /I "..\..\src" /I "..\..\src\regex" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "THREADLIB_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_INTERNAL_LUA" /D "USE_PLATFORM_ACCEL" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Stripping debug info...
PostBuild_Cmds=rebase -b 0x400000 -x . "../../uqm.exe"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\..\src" /I "..\..\src\regex" /D "DEBUG" /D "_DEBUG" /D "DEBUG_TRACK_SEM" /D "DEBUG_DCQ_THREADS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "THREADLIB_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_INTERNAL_LUA" /D "USE_PLATFORM_ACCEL" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"UrQuanMasters.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_NoAccel"
# PROP BASE Intermediate_Dir "Debug_NoAccel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_NoAccel"
# PROP Intermediate_Dir "Debug_NoAccel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\..\src" /I "..\..\src\regex" /D "DEBUG" /D "_DEBUG" /D "DEBUG_TRACK_SEM" /D "DEBUG_DCQ_THREADS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "THREADLIB_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_INTERNAL_LUA" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"UrQuanMasters.bsc"
# ADD BSC32 /nologo /o"UrQuanMasters.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_NoAccel"
# PROP BASE Intermediate_Dir "Release_NoAccel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_NoAccel"
# PROP Intermediate_Dir "Release_NoAccel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W2 /GX /Zi /O2 /I "..\..\src" /I "..\..\src\regex" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "THREADLIB_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_INTERNAL_LUA" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Stripping debug info...
PostBuild_Cmds=rebase -b 0x400000 -x . "../../uqm.exe"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "UrQuanMasters - Win32 Release"
# Name "UrQuanMasters - Win32 Debug"
# Name "UrQuanMasters - Win32 Debug NoAccel"
# Name "UrQuanMasters - Win32 Release NoAccel"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "getopt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\getopt\getopt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\getopt\getopt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\getopt\getopt1.c
# End Source File
# End Group
# Begin Group "regex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\regex\regcomp.ci
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regex.c
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regex.h
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regex_internal.ci
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regex_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regexec.ci
# End Source File
# End Group
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Group "callback"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\callback\alarm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback\alarm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback\async.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback\async.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback\callback.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback\callback.h
# End Source File
# End Group
# Begin Group "decomp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\decomp\lzdecode.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\decomp\lzencode.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\decomp\lzh.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\decomp\update.c
# End Source File
# End Group
# Begin Group "file"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\file\dirs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\file\files.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\file\filintrn.h
# End Source File
# End Group
# Begin Group "graphics"

# PROP Default_Filter ""
# Begin Group "sdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers_3dnow.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers_mmx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers_mmx.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\2xscalers_sse.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\biadv2x.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\bilinear2x.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\canvas.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\hq2x.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\nearest2x.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\opengl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\opengl.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\palette.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\palette.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\primitives.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\primitives.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\pure.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\pure.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\rotozoom.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\rotozoom.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\scaleint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\scalemmx.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\scalers.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\scalers.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\sdl_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\sdl_common.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\sdluio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\sdluio.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\sdl\triscan2x.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\graphics\bbox.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\bbox.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\boxint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\clipline.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\cmap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\cmap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\context.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\context.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\dcqueue.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\dcqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\drawable.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\drawable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\drawcmd.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\filegfx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\font.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\font.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\frame.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\gfx_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\gfx_common.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\gfxintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\gfxload.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\intersec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\loaddisp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\pixmap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\prim.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\resgfx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\tfb_draw.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\tfb_draw.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\tfb_prim.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\tfb_prim.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\widgets.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\graphics\widgets.h
# End Source File
# End Group
# Begin Group "heap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\heap\heap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\heap\heap.h
# End Source File
# End Group
# Begin Group "input"

# PROP Default_Filter ""
# Begin Group "sdl No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\input.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\input.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\keynames.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\keynames.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\vcontrol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\vcontrol.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\sdl\vcontrol_malloc.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\input\inpintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\input_common.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\input\input_common.h
# End Source File
# End Group
# Begin Group "list"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\list\list.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\list\list.h
# End Source File
# End Group
# Begin Group "lua"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\lua\lapi.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lapi.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lauxlib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lauxlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lbaselib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lbitlib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lcode.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lcode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lctype.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lctype.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ldebug.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ldebug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ldo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ldo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ldump.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lfunc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lfunc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lgc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lgc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\llex.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\llex.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\llimits.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lmathlib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lmem.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lmem.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lobject.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lobject.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lopcodes.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lopcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lparser.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lparser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lstate.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lstate.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lstring.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lstring.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lstrlib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ltable.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ltable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ltablib.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ltm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\ltm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lua.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\luaconf.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lualib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lundump.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lundump.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lvm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lvm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lzio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\lua\lzio.h
# End Source File
# End Group
# Begin Group "luauqm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\luauqm\luauqm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\luauqm\luauqm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\luauqm\scriptres.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\luauqm\scriptres.h
# End Source File
# End Group
# Begin Group "log"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\log\loginternal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\log\msgbox.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\log\msgbox_win.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\log\uqmlog.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\log\uqmlog.h
# End Source File
# End Group
# Begin Group "math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\math\mthintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\math\random.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\math\random.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\math\random2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\math\sqrt.c
# End Source File
# End Group
# Begin Group "md5"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\md5\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\md5\md5.h
# End Source File
# End Group
# Begin Group "memory"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\memory\w_memlib.c
# End Source File
# End Group
# Begin Group "resource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\resource\direct.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\filecntl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\getres.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\index.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\loadres.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\propfile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\propfile.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\resinit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\resintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\stringbank.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\resource\stringbank.h
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Group "openal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\sound\openal\audiodrv_openal.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\openal\audiodrv_openal.h
# End Source File
# End Group
# Begin Group "decoders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\aiffaud.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\aiffaud.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\decoder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\dukaud.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\dukaud.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\modaud.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\modaud.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\oggaud.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\oggaud.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\wav.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\decoders\wav.h
# End Source File
# End Group
# Begin Group "mixer"

# PROP Default_Filter ""
# Begin Group "mixsdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\sdl\audiodrv_sdl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\sdl\audiodrv_sdl.h
# End Source File
# End Group
# Begin Group "nosound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\nosound\audiodrv_nosound.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\nosound\audiodrv_nosound.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\mixer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\mixer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\mixer\mixerint.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\sound\audiocore.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\audiocore.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\fileinst.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\music.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\resinst.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\sfx.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\sndintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\sound.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\stream.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\stream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\trackint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\trackplayer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sound\trackplayer.h
# End Source File
# End Group
# Begin Group "strings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\strings\getstr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\sfileins.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\sresins.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\stringhashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\stringhashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\strings.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\strintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strings\unicode.c
# End Source File
# End Group
# Begin Group "video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\video\dukvid.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\dukvid.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\legacyplayer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\vfileins.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\video.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\video.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\videodec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\videodec.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\vidintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\vidplayer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\vidplayer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\video\vresins.c
# End Source File
# End Group
# Begin Group "threads"

# PROP Default_Filter ""
# Begin Group "sdl No. 3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\threads\sdl\sdlthreads.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\threads\sdl\sdlthreads.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\threads\thrcommon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\threads\thrcommon.h
# End Source File
# End Group
# Begin Group "time"

# PROP Default_Filter ""
# Begin Group "sdl No. 4"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\time\sdl\sdltime.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\time\sdl\sdltime.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\time\timecommon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\time\timecommon.h
# End Source File
# End Group
# Begin Group "task"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\task\tasklib.c
# End Source File
# End Group
# Begin Group "uio"

# PROP Default_Filter ""
# Begin Group "stdio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\uio\stdio\stdio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\stdio\stdio.h
# End Source File
# End Group
# Begin Group "zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\uio\zip\zip.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\zip\zip.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\uio\charhashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\charhashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\debug.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\defaultfs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\defaultfs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\fileblock.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\fileblock.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\fstypes.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\fstypes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\getint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\gphys.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\gphys.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\hashtable.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\io.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\io.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\ioaux.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\ioaux.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\iointrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\match.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\match.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\mem.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\memdebug.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\memdebug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\mount.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\mount.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\mounttree.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\mounttree.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\paths.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\paths.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\physical.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\physical.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\types.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\uioport.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\uiostream.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\uiostream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\uioutils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\uioutils.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio\utils.h
# End Source File
# End Group
# Begin Group "mikmod"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\mikmod\drv_nos.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\load_it.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\load_mod.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\load_s3m.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\load_stm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\load_xm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mdreg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mdriver.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mikmod.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mikmod_build.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mikmod_internals.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mloader.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mlreg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mlutil.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mmalloc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mmerror.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mmio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mplayer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\munitrk.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\mwav.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\npertab.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\sloader.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\virtch.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\virtch2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mikmod\virtch_common.c
# End Source File
# End Group
# Begin Group "network"

# PROP Default_Filter ""
# Begin Group "connect"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\network\connect\connect.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\connect\connect.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\connect\listen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\connect\listen.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\connect\resolve.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\connect\resolve.h
# End Source File
# End Group
# Begin Group "netmanager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\ndesc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\ndesc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\ndindex.ci
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\netmanager.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\netmanager_common.ci
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\netmanager_win.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netmanager\netmanager_win.h
# End Source File
# End Group
# Begin Group "socket"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\libs\network\socket\socket.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\socket\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\socket\socket_win.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\socket\socket_win.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\network\bytesex.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netport.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\netport.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\network.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\network_win.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\network\wspiapiwrap.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\libs\alarm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\async.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\callback.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\compiler.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\declib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\file.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\gfxlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\heap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\inplib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\list.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\log.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\mathlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\memlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\misc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\net.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\reslib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\sndlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\strlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\tasklib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\threadlib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\timelib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\uio.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\unicode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libs\vidlib.h
# End Source File
# End Group
# Begin Group "uqm"

# PROP Default_Filter ""
# Begin Group "comm"

# PROP Default_Filter ""
# Begin Group "arilou.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\arilou\arilouc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\arilou\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\arilou\strings.h
# End Source File
# End Group
# Begin Group "blackur.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\blackur\blackurc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\blackur\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\blackur\strings.h
# End Source File
# End Group
# Begin Group "chmmr.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\chmmr\chmmrc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\chmmr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\chmmr\strings.h
# End Source File
# End Group
# Begin Group "comandr.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\comandr\comandr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\comandr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\comandr\strings.h
# End Source File
# End Group
# Begin Group "druuge.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\druuge\druugec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\druuge\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\druuge\strings.h
# End Source File
# End Group
# Begin Group "ilwrath.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\ilwrath\ilwrathc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\ilwrath\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\ilwrath\strings.h
# End Source File
# End Group
# Begin Group "melnorm.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\melnorm\melnorm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\melnorm\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\melnorm\strings.h
# End Source File
# End Group
# Begin Group "mycon.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\mycon\myconc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\mycon\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\mycon\strings.h
# End Source File
# End Group
# Begin Group "orz.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\orz\orzc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\orz\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\orz\strings.h
# End Source File
# End Group
# Begin Group "pkunk.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\pkunk\pkunkc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\pkunk\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\pkunk\strings.h
# End Source File
# End Group
# Begin Group "rebel.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\rebel\rebel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\rebel\strings.h
# End Source File
# End Group
# Begin Group "shofixt.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\shofixt\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\shofixt\shofixt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\shofixt\strings.h
# End Source File
# End Group
# Begin Group "slyhome.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyhome\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyhome\slyhome.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyhome\strings.h
# End Source File
# End Group
# Begin Group "slyland.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyland\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyland\slyland.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\slyland\strings.h
# End Source File
# End Group
# Begin Group "spahome.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\spahome\spahome.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\spahome\strings.h
# End Source File
# End Group
# Begin Group "spathi.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\spathi\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\spathi\spathic.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\spathi\strings.h
# End Source File
# End Group
# Begin Group "starbas.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\starbas\starbas.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\starbas\strings.h
# End Source File
# End Group
# Begin Group "supox.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\supox\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\supox\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\supox\supoxc.c
# End Source File
# End Group
# Begin Group "syreen.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\syreen\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\syreen\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\syreen\syreenc.c
# End Source File
# End Group
# Begin Group "talkpet.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\talkpet\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\talkpet\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\talkpet\talkpet.c
# End Source File
# End Group
# Begin Group "thradd.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\thradd\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\thradd\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\thradd\thraddc.c
# End Source File
# End Group
# Begin Group "umgah.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\umgah\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\umgah\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\umgah\umgahc.c
# End Source File
# End Group
# Begin Group "urquan.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\urquan\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\urquan\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\urquan\urquanc.c
# End Source File
# End Group
# Begin Group "utwig.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\utwig\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\utwig\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\utwig\utwigc.c
# End Source File
# End Group
# Begin Group "vux.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\vux\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\vux\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\vux\vuxc.c
# End Source File
# End Group
# Begin Group "yehat.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\yehat\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\yehat\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\yehat\yehatc.c
# End Source File
# End Group
# Begin Group "zoqfot.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\comm\zoqfot\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\zoqfot\strings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm\zoqfot\zoqfotc.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\comm\commall.h
# End Source File
# End Group
# Begin Group "lua (uqm)"

# PROP Default_Filter ""
# Begin Group "luafuncs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\commfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\commfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\customfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\customfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\eventfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\eventfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\logfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\logfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\statefuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luafuncs\statefuncs.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\lua\luacomm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luacomm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luadebug.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luadebug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luaevent.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luaevent.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luainit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luainit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luastate.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\lua\luastate.h
# End Source File
# End Group
# Begin Group "planets"

# PROP Default_Filter ""
# Begin Group "generate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genall.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genand.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genburv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genchmmr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gencol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gendefault.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gendefault.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gendru.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genilw.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genmel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genmyc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genorz.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genpet.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genpku.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genrain.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gensam.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genshof.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gensly.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gensol.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genspa.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gensup.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gensyr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genthrad.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\gentrap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genutw.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genvault.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genvux.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genwreck.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genyeh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genzfpscout.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate\genzoq.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\planets\calc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\cargo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\devices.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\elemdata.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\generate.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\gentopo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\lander.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\lander.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\lifeform.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\orbits.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\oval.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\pl_stuff.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\plandata.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\planets.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\planets.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\plangen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\pstarmap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\report.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\roster.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\scan.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\scan.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\solarsys.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\solarsys.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\sundata.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\planets\surface.c
# End Source File
# End Group
# Begin Group "ships"

# PROP Default_Filter ""
# Begin Group "androsyn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\androsyn\androsyn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\androsyn\androsyn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\androsyn\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\androsyn\resinst.h
# End Source File
# End Group
# Begin Group "arilou"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\arilou\arilou.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\arilou\arilou.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\arilou\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\arilou\resinst.h
# End Source File
# End Group
# Begin Group "blackurq"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\blackurq\blackurq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\blackurq\blackurq.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\blackurq\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\blackurq\resinst.h
# End Source File
# End Group
# Begin Group "chenjesu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\chenjesu\chenjesu.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chenjesu\chenjesu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chenjesu\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chenjesu\resinst.h
# End Source File
# End Group
# Begin Group "chmmr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\chmmr\chmmr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chmmr\chmmr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chmmr\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\chmmr\resinst.h
# End Source File
# End Group
# Begin Group "druuge"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\druuge\druuge.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\druuge\druuge.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\druuge\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\druuge\resinst.h
# End Source File
# End Group
# Begin Group "human"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\human\human.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\human\human.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\human\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\human\resinst.h
# End Source File
# End Group
# Begin Group "ilwrath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\ilwrath\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\ilwrath\ilwrath.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\ilwrath\ilwrath.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\ilwrath\resinst.h
# End Source File
# End Group
# Begin Group "lastbat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\lastbat\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\lastbat\lastbat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\lastbat\lastbat.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\lastbat\resinst.h
# End Source File
# End Group
# Begin Group "melnorme"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\melnorme\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\melnorme\melnorme.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\melnorme\melnorme.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\melnorme\resinst.h
# End Source File
# End Group
# Begin Group "mmrnmhrm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\mmrnmhrm\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mmrnmhrm\mmrnmhrm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mmrnmhrm\mmrnmhrm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mmrnmhrm\resinst.h
# End Source File
# End Group
# Begin Group "mycon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\mycon\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mycon\mycon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mycon\mycon.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\mycon\resinst.h
# End Source File
# End Group
# Begin Group "orz"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\orz\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\orz\orz.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\orz\orz.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\orz\resinst.h
# End Source File
# End Group
# Begin Group "pkunk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\pkunk\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\pkunk\pkunk.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\pkunk\pkunk.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\pkunk\resinst.h
# End Source File
# End Group
# Begin Group "probe"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\probe\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\probe\probe.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\probe\probe.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\probe\resinst.h
# End Source File
# End Group
# Begin Group "shofixti"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\shofixti\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\shofixti\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\shofixti\shofixti.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\shofixti\shofixti.h
# End Source File
# End Group
# Begin Group "sis_ship"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\sis_ship\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\sis_ship\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\sis_ship\sis_ship.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\sis_ship\sis_ship.h
# End Source File
# End Group
# Begin Group "slylandr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\slylandr\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\slylandr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\slylandr\slylandr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\slylandr\slylandr.h
# End Source File
# End Group
# Begin Group "spathi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\spathi\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\spathi\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\spathi\spathi.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\spathi\spathi.h
# End Source File
# End Group
# Begin Group "supox"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\supox\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\supox\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\supox\supox.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\supox\supox.h
# End Source File
# End Group
# Begin Group "syreen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\syreen\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\syreen\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\syreen\syreen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\syreen\syreen.h
# End Source File
# End Group
# Begin Group "thradd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\thradd\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\thradd\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\thradd\thradd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\thradd\thradd.h
# End Source File
# End Group
# Begin Group "umgah"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\umgah\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\umgah\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\umgah\umgah.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\umgah\umgah.h
# End Source File
# End Group
# Begin Group "urquan"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\urquan\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\urquan\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\urquan\urquan.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\urquan\urquan.h
# End Source File
# End Group
# Begin Group "utwig"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\utwig\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\utwig\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\utwig\utwig.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\utwig\utwig.h
# End Source File
# End Group
# Begin Group "vux"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\vux\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\vux\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\vux\vux.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\vux\vux.h
# End Source File
# End Group
# Begin Group "yehat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\yehat\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\yehat\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\yehat\yehat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\yehat\yehat.h
# End Source File
# End Group
# Begin Group "zoqfot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\ships\zoqfot\icode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\zoqfot\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\zoqfot\zoqfot.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ships\zoqfot\zoqfot.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\ships\ship.h
# End Source File
# End Group
# Begin Group "supermelee"

# PROP Default_Filter ""
# Begin Group "netplay"

# PROP Default_Filter ""
# Begin Group "proto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\npconfirm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\npconfirm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\ready.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\ready.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\reset.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\proto\reset.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\checkbuf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\checkbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\checksum.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\checksum.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\crc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\crc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\nc_connect.ci
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netconnection.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netconnection.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netinput.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netinput.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netmelee.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netmelee.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netmisc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netoptions.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netoptions.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netplay.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netrcv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netrcv.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netsend.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netsend.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netstate.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\netstate.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\notify.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\notify.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\notifyall.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\notifyall.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packet.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packet.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packethandlers.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packethandlers.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packetq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packetq.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packetsenders.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\netplay\packetsenders.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\buildpick.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\buildpick.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\loadmele.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\loadmele.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\melee.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\melee.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\meleesetup.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\meleesetup.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\pickmele.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\supermelee\pickmele.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\uqm\battle.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\battle.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\battlecontrols.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\battlecontrols.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\border.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\build.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\build.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cleanup.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\clock.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\clock.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cnctdlg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cnctdlg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\coderes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\collide.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\collide.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\colors.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\comm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\commanim.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\commanim.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\commglue.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\commglue.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\confirm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cons_res.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cons_res.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\controls.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\corecode.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\credits.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\credits.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\cyborg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\demo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\displist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\displist.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\dummy.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\dummy.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\element.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\encount.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\encount.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\flash.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\flash.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\fmv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\fmv.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\galaxy.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gameev.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gameev.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gameinp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gameopt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gameopt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gamestr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gendef.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gendef.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\getchar.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\globdata.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\globdata.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\gravity.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\grpinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\grpinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\grpintrn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\hyper.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\hyper.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ifontres.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ikey_con.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\init.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\init.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\intel.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\intel.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\intro.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ipdisp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ipdisp.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\isndres.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\load.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\load_legacy.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\loadship.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\master.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\master.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\menu.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\menustat.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\nameref.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\oscill.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\oscill.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\outfit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\pickship.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\pickship.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\plandata.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\process.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\process.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\races.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\resinst.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\restart.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\restart.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\restypes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\save.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\save.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\settings.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\settings.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\setup.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\setup.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\setupmenu.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\setupmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ship.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\ship.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\shipcont.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\shipstat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\shipyard.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\sis.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\sis.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\sounds.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\sounds.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starbase.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starbase.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starcon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starcon.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starmap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\starmap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\state.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\state.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\status.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\status.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\tactrans.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\tactrans.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\trans.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\units.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\uqmdebug.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\uqmdebug.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\util.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\util.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\velocity.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\velocity.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\weapon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm\weapon.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\config_vc6.h
# End Source File
# Begin Source File

SOURCE=..\..\src\endian_uqm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\options.c
# End Source File
# Begin Source File

SOURCE=..\..\src\options.h
# End Source File
# Begin Source File

SOURCE=..\..\src\port.c
# End Source File
# Begin Source File

SOURCE=..\..\src\port.h
# End Source File
# Begin Source File

SOURCE=..\..\src\types.h
# End Source File
# Begin Source File

SOURCE=..\..\src\uqm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\uqmversion.h
# End Source File
# End Group
# Begin Group "Doc"

# PROP Default_Filter ""
# Begin Group "Devel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\devel\aniformat
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\checklist
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\debug
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\dialogs
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\files
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\fontres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\generate
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxlib
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxversions
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\glossary
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\input
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\musicres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\pkgformat
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\planetrender
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\planetrotate
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\plugins
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\resources
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\script
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\sfx
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\strtab
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\threads
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\timing
# End Source File
# End Group
# Begin Group "Users"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\users\manual.txt
# End Source File
# Begin Source File

SOURCE=..\..\doc\users\unixinstall
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\BUGS
# End Source File
# Begin Source File

SOURCE=..\..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\..\TODO
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\res\kohr-ah1.ico"
# End Source File
# Begin Source File

SOURCE=..\..\src\res\sis1.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\res\starcon2.ico
# End Source File
# Begin Source File

SOURCE="..\..\src\res\ur-quan-icon-alpha.ico"
# End Source File
# Begin Source File

SOURCE="..\..\src\res\ur-quan-icon-std.ico"
# End Source File
# Begin Source File

SOURCE="..\..\src\res\ur-quan1.ico"
# End Source File
# Begin Source File

SOURCE="..\..\src\res\ur-quan2.ico"
# End Source File
# Begin Source File

SOURCE=..\..\src\res\UrQuanMasters.rc
# End Source File
# End Group
# End Target
# End Project
