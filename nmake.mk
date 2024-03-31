EXEEXT    = .exe
OBJEXT    = .obj
LIBEXT    = .lib
RM        = del /Q /F
OUT       = -Fe

CL        = cl -nologo
CC        = $(CL)
CXX       = $(CL)
AR        = lib -nologo
CLANG     = clang

CFLAGS    = -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN
CXXFLAGS  = $(CFLAGS) -EHsc -std:c++latest
LDFLAGS   = -link -subsystem:console -release
