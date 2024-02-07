CCAUX     := $(CC)

STDCXX    ?= c++11
SOFLAGS   := -DBUILDING_DLL
OBJEXT    := o

ifneq ($(CLANG_CL),)
CC         = $(CLANG_CL)
CXX        = $(CLANG_CL)
CFLAGS    := -W3 -O2 -DWIN32_LEAN_AND_MEAN
CXXFLAGS  := $(CFLAGS) -EHsc
LDFLAGS   := -fuse-ld=lld -link -subsystem:console -libpath:$(shell echo $$LIB | sed 's|;| -libpath:|g')
LIBEXT    := lib

else

LIBEXT    := a
AR        := ar
ARFLAGS   := cr

ifneq ($(shell echo $(CXX) | grep mingw),)
CFLAGS    := -Wall -Wextra -O3 -DWIN32_LEAN_AND_MEAN
CXXFLAGS  := $(CFLAGS) -std=$(STDCXX)
MUNICODE  := -municode
LDFLAGS   := -static -s
else
CFLAGS    := -Wall -Wextra -O3 -D_GNU_SOURCE
CXXFLAGS  := $(CFLAGS) -std=$(STDCXX)
SOFLAGS   += -fvisibility=hidden -fPIC
LDFLAGS   := -s
LDFLAGS   += -ldl
endif

endif

define compile_cc
$(CC) $(CFLAGS) $(2) -o $@ $(1) $(LDFLAGS) $(3)
endef
define compile_cxx
$(CXX) $(CXXFLAGS) $(2) -o $@ $(1) $(LDFLAGS) $(3)
endef