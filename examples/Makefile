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
CFLAGS    := -Wall -Wextra -O3 -Wno-deprecated-declarations -D_GNU_SOURCE
CXXFLAGS  := $(CFLAGS) -std=c++11
LIB       := libhelloworld.so.0
LIBX      := libhelloworld.so
SOFLAGS   += -fvisibility=hidden -fPIC
LDSOFLAGS := -shared -Wl,-soname,$(LIB)
LDFLAGS   := -Wl,-rpath,'$$ORIGIN' -s
LIBS      := -ldl
BINS2      = notify appindicator
endif
endif

# test whether everything is handled
# correctly if a symbol was not found
ifneq ($(TEST_UNDEFINED_SYMBOL),)
CFLAGS   += -DTEST_UNDEFINED_SYMBOL
CXXFLAGS += -DTEST_UNDEFINED_SYMBOL
BINS2     =
endif


.PHONY: all clean run_tests run_tests2

all: $(BINS) $(BINS2)

clean:
	-rm -f $(BINS) notify appindicator
	-rm -f *.exe *.dll *.dylib *.so *.so.* *.lib *.a
	-rm -f example_*.h example_*.hpp

run_tests: $(BINS)
	@for f in $^; do \
		echo "./$$f"; ./$$f; echo "return value: $$?"; echo ""; \
	done

run_tests2: $(BINS2)
	@for f in $^; do \
		echo "./$$f"; ./$$f; echo "return value: $$?"; echo ""; \
	done


$(BINS): $(LIBX)

libhelloworld.so: $(LIB)
	ln -fs $< $@

libhelloworld.dll.a: $(LIB)
libhelloworld.lib: $(LIB)

$(LIB): helloworld.c
	$(CC) $(SOFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDSOFLAGS)

example_1: example_1.c example_1.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LIBS)

example_auto_free: example_auto_free.c example_auto_free.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LIBS)

example_autoload: example_autoload.c example_autoload.h
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LIBS)

example_cxx: example_cxx.cpp example_cxx.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS) $(LIBS)

example_cxx_autoload: example_cxx_autoload.cpp example_cxx_autoload.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS) $(LIBS)

example_wide: example_wide.c example_wide.h
	$(CC) $(CFLAGS) -DUNICODE -D_UNICODE $(MUNICODE) $@.c -o $@ $(LDFLAGS)

example_cxx_wide: example_cxx_wide.cpp example_cxx_wide.hpp
	$(CXX) $(CXXFLAGS) $@.cpp -o $@ $(LDFLAGS)

appindicator: appindicator.c
	$(GENDLOPEN) -i appindicator.txt --name=xappindicator > example_appindicator_appindicator.h
	$(GENDLOPEN) -i appindicator-gobject.txt --name=xgobject > example_appindicator_gobject.h
	$(GENDLOPEN) -i appindicator-gtk.txt --name=xgtk > example_appindicator_gtk.h
	$(CC) $(CFLAGS) -DUSE_DLOPEN=1 `pkg-config --cflags appindicator-0.1` $@.c -o $@ $(LDFLAGS) $(LIBS)

notify: notify.c
	$(GENDLOPEN) -i notify.txt --name=xnotify > example_notify_notify.h
	$(GENDLOPEN) -i notify-gobject.txt --name=xgobject > example_notify_gobject.h
	$(CC) $(CFLAGS) -DUSE_DLOPEN=1 `pkg-config --cflags libnotify` $@.c -o $@ $(LDFLAGS) $(LIBS)

example_%.h: helloworld.txt
	$(GENDLOPEN) -i $^ > $@

example_%.hpp: helloworld.txt
	$(GENDLOPEN) -x -i $^ > $@

