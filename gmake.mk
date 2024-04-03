RM        = -rm -f
OUT       = -o
OBJEXT    = .o
LIBEXT    = .a

STDCXX   ?= c++11
CLANG    ?= clang

AR       ?= ar
ARFLAGS  := cr


ifneq ($(CLANG_CL),)
# clang-cl
CC        = $(CLANG_CL)
CXX       = $(CLANG_CL)
CFLAGS   := -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN
CXXFLAGS := -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN -std:c++latest -EHsc
SOCFLAGS := -DBUILDING_DLL
LDFLAGS  := -fuse-ld=lld -link -subsystem:console -libpath:$(shell echo $$LIB | sed 's|;| -libpath:|g')
LIBEXT    = .lib
#EXEEXT    = .exe

else

ifneq ($(shell $(CC) -dumpmachine | grep mingw),)
# MinGW
#EXEEXT    = .exe
CFLAGS   := -Wall -Wextra -O3 -I. -DWIN32_LEAN_AND_MEAN
CXXFLAGS := -Wall -Wextra -O3 -I. -DWIN32_LEAN_AND_MEAN -std=$(STDCXX)
SOCFLAGS := -DBUILDING_DLL
LDFLAGS  := -s -static
else
# *nix
CFLAGS   := -Wall -Wextra -O3 -I.
CXXFLAGS := -Wall -Wextra -O3 -I. -std=$(STDCXX)
SOCFLAGS := -fvisibility=hidden -fPIC
LDFLAGS  := -s
endif

LIBEXT   := .a

endif
