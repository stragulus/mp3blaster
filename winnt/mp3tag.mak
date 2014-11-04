# Microsoft Developer Studio Generated NMAKE File, Based on mp3tag.dsp
!IF "$(CFG)" == ""
CFG=mp3tag - Win32 Debug
!MESSAGE No configuration specified. Defaulting to mp3tag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "mp3tag - Win32 Release" && "$(CFG)" != "mp3tag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mp3tag.mak" CFG="mp3tag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mp3tag - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mp3tag - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "mp3tag - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mp3tag.exe"

!ELSE 

ALL : "$(OUTDIR)\mp3tag.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\id3parse.obj"
	-@erase "$(INTDIR)\mp3tag.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\mp3tag.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)\mp3tag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mp3tag.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\mp3tag.pdb" /machine:I386 /out:"$(OUTDIR)\mp3tag.exe" 
LINK32_OBJS= \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\id3parse.obj" \
	"$(INTDIR)\mp3tag.obj"

"$(OUTDIR)\mp3tag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mp3tag - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mp3tag.exe" "$(OUTDIR)\mp3tag.bsc"

!ELSE 

ALL : "$(OUTDIR)\mp3tag.exe" "$(OUTDIR)\mp3tag.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\id3parse.obj"
	-@erase "$(INTDIR)\id3parse.sbr"
	-@erase "$(INTDIR)\mp3tag.obj"
	-@erase "$(INTDIR)\mp3tag.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\mp3tag.bsc"
	-@erase "$(OUTDIR)\mp3tag.exe"
	-@erase "$(OUTDIR)\mp3tag.ilk"
	-@erase "$(OUTDIR)\mp3tag.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mp3tag.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mp3tag.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\id3parse.sbr" \
	"$(INTDIR)\mp3tag.sbr"

"$(OUTDIR)\mp3tag.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\mp3tag.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mp3tag.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\id3parse.obj" \
	"$(INTDIR)\mp3tag.obj"

"$(OUTDIR)\mp3tag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "mp3tag - Win32 Release" || "$(CFG)" == "mp3tag - Win32 Debug"
SOURCE=.\getopt.c
DEP_CPP_GETOP=\
	".\getopt.h"\
	

!IF  "$(CFG)" == "mp3tag - Win32 Release"


"$(INTDIR)\getopt.obj" : $(SOURCE) $(DEP_CPP_GETOP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mp3tag - Win32 Debug"


"$(INTDIR)\getopt.obj"	"$(INTDIR)\getopt.sbr" : $(SOURCE) $(DEP_CPP_GETOP)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\id3parse.cpp
DEP_CPP_ID3PA=\
	".\id3parse.h"\
	

!IF  "$(CFG)" == "mp3tag - Win32 Release"


"$(INTDIR)\id3parse.obj" : $(SOURCE) $(DEP_CPP_ID3PA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mp3tag - Win32 Debug"


"$(INTDIR)\id3parse.obj"	"$(INTDIR)\id3parse.sbr" : $(SOURCE) $(DEP_CPP_ID3PA)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\mp3tag.cpp
DEP_CPP_MP3TA=\
	".\getopt.h"\
	".\id3parse.h"\
	

!IF  "$(CFG)" == "mp3tag - Win32 Release"


"$(INTDIR)\mp3tag.obj" : $(SOURCE) $(DEP_CPP_MP3TA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mp3tag - Win32 Debug"


"$(INTDIR)\mp3tag.obj"	"$(INTDIR)\mp3tag.sbr" : $(SOURCE) $(DEP_CPP_MP3TA)\
 "$(INTDIR)"


!ENDIF 


!ENDIF 

