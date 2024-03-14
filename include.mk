CCAUX     := $(CC)
CLANG     := clang

STDCXX    ?= c++11
SOFLAGS   := -DBUILDING_DLL
OBJEXT    := o

ifneq ($(CLANG_CL),)
CC         = $(CLANG_CL)
CXX        = $(CLANG_CL)
CFLAGS    := -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN
CXXFLAGS  := $(CFLAGS) -std:$(STDCXX) -EHsc
LDFLAGS   := -fuse-ld=lld -link -subsystem:console -libpath:$(shell echo $$LIB | sed 's|;| -libpath:|g')
LIBEXT    := lib

else

LIBEXT    := a
AR        := ar
ARFLAGS   := cr

ifneq ($(shell echo $(CXX) | grep mingw),)
CFLAGS    := -Wall -Wextra -O3 -I. -DWIN32_LEAN_AND_MEAN
CXXFLAGS  := $(CFLAGS) -std=$(STDCXX)
MUNICODE  := -municode
LDFLAGS   := -static -s
else
CFLAGS    := -Wall -Wextra -O3 -I.
CXXFLAGS  := $(CFLAGS) -std=$(STDCXX)
SOFLAGS   += -fvisibility=hidden -fPIC
LDFLAGS   := -s
endif

endif

define compile_cc
$(CC) $(CFLAGS) $(2) -o $@ $(1) $(LDFLAGS) $(3)
endef
define compile_cxx
$(CXX) $(CXXFLAGS) $(2) -o $@ $(1) $(LDFLAGS) $(3)
endef
