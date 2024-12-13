!IF !DEFINED( V )
V = 0
!ENDIF

!IF "$(V)" == "0" || "$(V)" == ""
silence = 2>NUL
.SILENT:
!ENDIF

NOLOGO = -nologo
STDVER = c++20

CC       = cl $(NOLOGO)
CXX      = cl $(NOLOGO)
LIB_EXE  = lib $(NOLOGO)
LEX      = flex
CPPFLAGS = -DWIN32_LEAN_AND_MEAN
CFLAGS   = -W4 -O2 -I.
CXXFLAGS = -W4 -O2 -I. -std:$(STDVER) -EHsc
LFLAGS   = -subsystem:console -release

JOBS = 4

# top source directory
s = ..

sources = $(s)\src\lex.yy.c $(s)\src\*.cpp


all: gendlopen.exe

clean:
	-del /q *.exe *.obj template.h $(silence)


## main program ##

#$(s)\src\data.cpp: template.h

#template.h: gen_template_h.exe
#	$** $(s)\src\templates

#$(s)\src\lex.yy.c: $(s)\src\mylex.l
#	$(LEX) --noline $**

#gen_template_h.exe: $(s)\src\gen_template_h.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) $** -Fe$@ -link $(LFLAGS)

gendlopen.exe: $(s)\src\lex.yy.c $(s)\src\*.cpp
	$(CXX) -MP$(JOBS) $(CXXFLAGS) $(CPPFLAGS) $** -Fe$@ -link $(LFLAGS)

