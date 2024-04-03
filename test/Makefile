# This one Makefile works with Microsoft nmake and GNU make.
# They use different conditional syntax, but each can be
# nested and inverted within the other.

STDCXX = c++11

all: default


ifdef MAKEDIR: # gmake: false; nmake: unused target
!ifdef MAKEDIR # gmake: not seen; nmake: true

#
# Microsoft nmake
#

!include ..\nmake.mk


BINS2     = c_win32_wide$(EXEEXT) cxx_win32_wide$(EXEEXT)
DLL       = libhelloworld-0.dll
IMPLIB    = libhelloworld.lib
SOCFLAGS  = -DBUILDING_DLL
SOLDFLAGS = -LD $(LDFLAGS) -dll -implib:$(IMPLIB)
GENDLOPEN = ..\src\gendlopen.exe
LIBOUT    = -out:


!else # and now the other
else

#
# GNU make
#

include ../gmake.mk


ifneq ($(CLANG_CL),)
# clang-cl
BINS2      = c_win32_wide$(EXEEXT) cxx_win32_wide$(EXEEXT)
DLL        = libhelloworld-0.dll
IMPLIB     = libhelloworld.lib
SOLDFLAGS := $(LDFLAGS) -dll -implib:$(IMPLIB)
else

ifneq ($(shell $(CC) -dumpmachine | grep mingw),)

# MinGW
BINS2      = c_win32_wide$(EXEEXT) cxx_win32_wide$(EXEEXT)
DLL        = libhelloworld-0.dll
IMPLIB     = libhelloworld.dll.a
SOLDFLAGS := -shared -Wl,--out-implib,$(IMPLIB)

else

# other/ELF
DLL        = libhelloworld.so.0
IMPLIB     = libhelloworld.so
SOLDFLAGS := -shared -Wl,-soname,$(DLL)
#LIBDL     = -ldl
LDFLAGS   += $(LIBDL) -Wl,-rpath,'$$ORIGIN'

ifneq ($(shell $(CC) -dumpmachine | grep gnu),)
CFLAGS    += -D_GNU_SOURCE
CXXFLAGS  += -D_GNU_SOURCE
endif

endif    # MINGW
endif    # CLANG_CL


GENDLOPEN = ../src/gendlopen


.PHONY: all default clean

endif    # gmake: close condition; nmake: not seen
!endif : # gmake: unused target; nmake close conditional



BINS = \
	c_test$(EXEEXT) \
	c_auto_free$(EXEEXT) \
	c_autoload$(EXEEXT) \
	c_minimal$(EXEEXT) \
	c_multi$(EXEEXT) \
	cxx_test$(EXEEXT) \
	cxx_autoload$(EXEEXT) \
	cxx_minimal$(EXEEXT) \
	cxx_multi$(EXEEXT) \
	helloworld_example_dl$(EXEEXT) \
	$(NULL)

GENHDR = $(GENDLOPEN) -i helloworld.txt -f -o



# default target for both
default: $(BINS)

$(BINS): $(IMPLIB)

clean:
	$(RM) *.exe *.o *.obj *.a *.lib *.dll *.so *.exp
	$(RM) $(DLL) $(BINS)
	$(RM) c_*.h cxx_*.hpp c_multi.c cxx_multi.cpp
	$(RM) libhelloworld_dl.c libhelloworld_dl.h



# DLL

$(IMPLIB): $(DLL)

libhelloworld.so: $(DLL)
	ln -fs $< $@

libhelloworld.dll.a: $(DLL)
libhelloworld.lib: $(DLL)

$(DLL):
	$(CC) $(CFLAGS) $(SOCFLAGS) helloworld.c $(OUT)$@ $(SOLDFLAGS)



# C test files

c_test$(EXEEXT): c_test.h
	$(CC) $(CFLAGS) c_test.c $(OUT)$@ $(LDFLAGS)

c_auto_free$(EXEEXT): c_auto_free.h
	$(CC) $(CFLAGS) c_auto_free.c $(OUT)$@ $(LDFLAGS)

c_autoload$(EXEEXT): c_autoload.h
	$(CC) $(CFLAGS) c_autoload.c $(OUT)$@ $(LDFLAGS)

c_minimal$(EXEEXT): c_minimal.h
	$(CC) $(CFLAGS) c_minimal.c $(OUT)$@ $(LDFLAGS)

c_multi$(EXEEXT): c_multi.h
	$(CC) $(CFLAGS) c_multi_part1.c c_multi_part2.c c_multi.c $(OUT)$@ $(LDFLAGS)



# C++ test files

cxx_test$(EXEEXT): cxx_test.hpp
	$(CXX) $(CXXFLAGS) cxx_test.cpp $(OUT)$@ $(LDFLAGS)

cxx_autoload$(EXEEXT): cxx_autoload.hpp
	$(CXX) $(CXXFLAGS) cxx_autoload.cpp $(OUT)$@ $(LDFLAGS)

cxx_minimal$(EXEEXT): cxx_minimal.hpp
	$(CXX) $(CXXFLAGS) cxx_minimal.cpp $(OUT)$@ $(LDFLAGS)

cxx_multi$(EXEEXT): cxx_multi.hpp
	$(CXX) $(CXXFLAGS) cxx_multi_part1.cpp cxx_multi_part2.cpp cxx_multi.cpp $(OUT)$@ $(LDFLAGS)



# static fake library test

libhelloworld_dl$(OBJEXT): libhelloworld_dl.c

libhelloworld_dl$(LIBEXT): libhelloworld_dl$(OBJEXT)
	$(AR) $(ARFLAGS) $(LIBOUT)$@ libhelloworld_dl$(OBJEXT)

helloworld_example_dl$(EXEEXT): libhelloworld_dl$(LIBEXT)
	$(CC) $(CFLAGS) -DBUILDING_STATIC helloworld_example.c $(OUT)$@ libhelloworld_dl$(LIBEXT) $(LDFLAGS)



# generate headers

libhelloworld_dl.c:
	$(GENHDR) $@ -DBUILDING_STATIC -DGDO_ENABLE_AUTOLOAD \
	  --separate --include=helloworld.h --library=API:0:helloworld

c_test.h:
	$(GENHDR) $@

c_auto_free.h:
	$(GENHDR) $@ --skip-parameter-names

c_autoload.h:
	$(GENHDR) $@

c_minimal.h:
	$(GENHDR) $@ --format=minimal

c_multi.h:
	$(GENHDR) $@ --separate --include=helloworld.h

cxx_test.hpp:
	$(GENHDR) $@ --format=C++ --skip-parameter-names

cxx_autoload.hpp:
	$(GENHDR) $@ --format=C++ --library=API:0:helloworld

cxx_minimal.hpp:
	$(GENHDR) $@ --format=minimal-C++

cxx_multi.hpp:
	$(GENHDR) $@ --format=C++ --separate --include=helloworld.h

