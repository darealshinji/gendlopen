STDCXX  := c++11

include ../include.mk


BINS = \
	example_1 \
	example_autoload \
	example_auto_free \
	example_cxx \
	example_cxx_autoload \
	helloworld_example_dl \
	helloworld_example_nodl

ifneq ($(CLANG_CL),)
# clang-cl
BINS      += example_wide example_cxx_wide
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.lib
#CXXFLAGS  += -std:c++latest
LDSOFLAGS := -dll -implib:$(LIBX)
else

ifneq ($(shell echo $(CXX) | grep mingw),)
# MinGW
BINS      += example_wide example_cxx_wide
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.dll.a
CFLAGS    += -Wno-cast-function-type
CXXFLAGS  += -Wno-cast-function-type
LDSOFLAGS := -shared -Wl,--out-implib,$(LIBX)
else
# Linux and BSD
BINS2     := appindicator notify
LIB       := libhelloworld.so.0
LIBX      := libhelloworld.so
CFLAGS    += -Wno-deprecated-declarations
LDFLAGS   += -Wl,-rpath,'$$ORIGIN'
LDSOFLAGS := -shared -Wl,-soname,$(LIB)
endif

endif

GENDLOPEN := ../src/gendlopen

# test whether everything is handled
# correctly if a symbol was not found
ifneq ($(TEST_UNDEFINED_SYMBOL),)
CFLAGS   += -DTEST_UNDEFINED_SYMBOL
CXXFLAGS += -DTEST_UNDEFINED_SYMBOL
BINS2     =
endif

define gdox
$(GENDLOPEN) -i $(1).txt -n $(2) -l $(3) > example_$(1).h
endef


.PHONY: all clean run_tests run_tests2

all: $(BINS) $(BINS2)

clean:
	-rm -f $(BINS) notify appindicator
	-rm -f *.exe *.dll *.dylib *.so *.so.* *.lib *.a *.o *.obj
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

$(LIB):
	$(call compile_cc,helloworld.c,$(SOFLAGS),$(LDSOFLAGS))

libhelloworld_dl.$(LIBEXT): example_helloworld_static_dl.$(OBJEXT)
	$(AR) $(ARFLAGS) $@ $<

example_helloworld_static_dl.$(OBJEXT): example_helloworld_static_dl.h
	$(call compile_cc,example_helloworld_static_dl.c,-c)

example_1: example_1.h
	$(call compile_cc,$@.c)

example_auto_free: example_auto_free.h
	$(call compile_cc,$@.c)

example_autoload: example_autoload.h
	$(call compile_cc,$@.c)

example_cxx: example_cxx.hpp
	$(call compile_cxx,$@.cpp)

example_cxx_autoload: example_cxx_autoload.hpp
	$(call compile_cxx,$@.cpp)

example_wide: example_wide.h
	$(call compile_cc,$@.c,-DUNICODE -D_UNICODE $(MUNICODE))

example_cxx_wide: example_cxx_wide.hpp
	$(call compile_cxx,$@.cpp)

helloworld_example_nodl: helloworld_example.c
	$(call compile_cc,helloworld_example.c,,$(LIBX))

helloworld_example_dl: libhelloworld_dl.$(LIBEXT)
	$(call compile_cc,helloworld_example.c,-DBUILDING_STATIC,libhelloworld_dl.$(LIBEXT))

appindicator: appindicator.c
	$(call gdox,appindicator,xappindicator,'"libappindicator.so.1"') # test filename with hyphens
	$(call gdox,appindicator_gobject,xgobject,libgobject-2.0.so.0)
	$(call gdox,appindicator_gtk,xgtk,libgtk-x11-2.0.so.0)
	$(call compile_cc, $@.c, -DUSE_DLOPEN=1 `pkg-config --cflags appindicator-0.1`)

notify: notify.c
	$(call gdox,notify,xnotify,libnotify.so.4)
	$(call gdox,notify_gobject,xgobject,libgobject-2.0.so.0)
	$(call compile_cc, $@.c, -DUSE_DLOPEN=1 `pkg-config --cflags libnotify`)

example_%.h: helloworld.txt
	$(GENDLOPEN) -i $^ > $@

example_%.hpp: helloworld.txt
	$(GENDLOPEN) --format=C++ -i $^ > $@

