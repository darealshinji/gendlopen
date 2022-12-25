CFLAGS   ?= -Wall -O2 -I.
CXXFLAGS ?= -Wall -O2 -I.


# cross compiling support
ifneq "$(GCC_PREFIX)" ""
CC  := $(GCC_PREFIX)gcc
CXX := $(GCC_PREFIX)g++
endif

# Windows
ifneq "$(shell $(CC) -dumpmachine | grep 'mingw')" ""
EXEEXT = .exe
LDFLAGS ?= -static
endif

BINS     = test1$(EXEEXT) test2$(EXEEXT) test3$(EXEEXT)
GENHDRS  = test1.h test2.hpp test3.hpp
GDO     ?= ../gendlopen$(EXEEXT)


all: $(BINS)

clean:
	-rm -f $(GENHDRS) $(BINS)

test1.c: test1.h
test2.cpp: test2.hpp
test3.cpp: test3.hpp

test1.h:
	$(GDO) -i zlib.txt -n gendlopen_zlib > $@

test2.hpp:
	$(GDO) -i zlib.txt -n gendlopen_zlib -t cpp-static > $@

test3.hpp:
	$(GDO) -i zlib.txt -n gendlopen_zlib -t cpp > $@
