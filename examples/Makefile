BINS = \
	example_1 \
	example_autoload \
	example_auto_free \
	example_cxx \
	example_cxx_autoload

GENDLOPEN ?= ../build/src/gendlopen
SOFLAGS   := -DBUILDING_DLL

ifneq ($(CLANG_CL),)
CC         = $(CLANG_CL)
CXX        = $(CLANG_CL)
BINS      += example_wide example_cxx_wide
CFLAGS    := -W3 -O2 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS
CXXFLAGS  := $(CFLAGS) /EHsc
LDFLAGS   := -fuse-ld=lld -link -subsystem:console -libpath:$(shell echo $$LIB | sed 's|;| -libpath:|g')
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.lib
LDSOFLAGS := -dll -implib:$(LIBX)
else
ifneq ($(shell echo $(CXX) | grep mingw),)
BINS      += example_wide example_cxx_wide
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.dll.a
CFLAGS    := -Wall -O2 -DWIN32_LEAN_AND_MEAN
CXXFLAGS  := $(CFLAGS) -std=c++11
MUNICODE  := -municode
LDSOFLAGS := -shared -Wl,--out-implib,$(LIBX)
LDFLAGS   := -s
else
CFLAGS    := -Wall -Wextra -O3
CXXFLAGS  := $(CFLAGS) -std=c++11
LIB       := libhelloworld.so.0
LIBX      := libhelloworld.so
SOFLAGS   += -fvisibility=hidden -fPIC
LDSOFLAGS := -shared -Wl,-soname,$(LIB)
LDFLAGS   := -Wl,-rpath,'$$ORIGIN' -s
endif
endif



.PHONY: all clean run_tests

all: $(BINS)

clean:
	-rm -f $(BINS)
	-rm -f *.exe *.dll *.dylib *.so *.so.* *.lib *.a
	-rm -f example_*.h example_*.hpp

run_tests: $(BINS)
	@for f in $(BINS); do \
		echo "./$$f"; ./$$f; echo ""; \
	done


$(BINS): $(LIBX)

libhelloworld.so: libhelloworld.so.0
	ln -fs $< $@

libhelloworld.dll.a: libhelloworld-0.dll
libhelloworld.lib: libhelloworld-0.dll

$(LIB): helloworld.c
	$(CC) $(SOFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDSOFLAGS)

example_1: example_1.c example_1.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS)

example_auto_free: example_auto_free.c example_auto_free.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS)

example_autoload: example_autoload.c example_autoload.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS)

example_cxx: example_cxx.cpp example_cxx.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS)

example_cxx_autoload: example_cxx_autoload.cpp example_cxx_autoload.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS)

example_wide: example_wide.c example_wide.h
	$(CC) $(CFLAGS) -DUNICODE -D_UNICODE $(MUNICODE) $@.c -o $@ $(LDFLAGS)

example_cxx_wide: example_cxx_wide.cpp example_cxx_wide.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS)

example_%.h: helloworld.txt
	$(GENDLOPEN) -i $^ > $@

example_%.hpp: helloworld.txt
	$(GENDLOPEN) -x -i $^ > $@

