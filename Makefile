CFLAGS   ?= -Wall -O2
CXXFLAGS ?= -Wall -O2


# cross compiling support
ifneq "$(GCC_PREFIX)" ""
CC  := $(GCC_PREFIX)gcc
CXX := $(GCC_PREFIX)g++
endif

# Windows
ifneq "$(shell $(CC) -dumpmachine | grep 'mingw')" ""
EXEEXT = .exe
LIBS += -lrpcrt4
LDFLAGS ?= -s -static
endif

XXD     := xxd
GGO     := gengetopt
BIN      = gendlopen$(EXEEXT)
OBJS     = main.o gendlopen.o cmdline.o
GENHDRS  = c_header.h cxx_header.h lib_macros.h


all: $(BIN)

clean:
	-rm -f gendlopen gendlopen.exe
	-rm -f $(GENHDRS)
	-rm -f $(OBJS)

distclean: clean

maintainer-clean: distclean
	-rm -f cmdline.c cmdline.h

gendlopen.o: $(GENHDRS)
main.o: cmdline.h
cmdline.o: cmdline.c
cmdline.c: cmdline.h

cmdline.h: cmdline.ggo
	$(GGO) < $<

%.h: %.template
	$(XXD) -i < $< > $@

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

