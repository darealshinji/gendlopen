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
BINS3     := uuid_win
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.lib
#CXXFLAGS  += -std:c++latest
LDSOFLAGS := -dll -implib:$(LIBX)
else

ifneq ($(shell echo $(CXX) | grep mingw),)
# MinGW
BINS      += example_wide example_cxx_wide
BINS2     := uuid_win
LIB       := libhelloworld-0.dll
LIBX      := libhelloworld.dll.a
CFLAGS    += -Wno-cast-function-type
CXXFLAGS  += -Wno-cast-function-type
LDSOFLAGS := -shared -Wl,--out-implib,$(LIBX)
else
# *nix
BINS2     := notify appindicator uuid_nix
LIB       := libhelloworld.so.0
LIBX      := libhelloworld.so
CFLAGS    += -Wno-deprecated-declarations
LDFLAGS   += -ldl -Wl,-rpath,'$$ORIGIN'
LDSOFLAGS := -shared -Wl,-soname,$(LIB)
endif

endif

GENDLOPEN       := ../src/gendlopen
PARSE_CLANG_AST := ../src/parse-clang-ast

# test whether everything is handled
# correctly if a symbol was not found
ifneq ($(TEST_UNDEFINED_SYMBOL),)
CFLAGS   += -DTEST_UNDEFINED_SYMBOL
CXXFLAGS += -DTEST_UNDEFINED_SYMBOL
BINS2     =
endif

define toupper
$(shell printf '$(1)' | tr a-z A-Z)
endef

# -D$(call toupper,$(2))_ENABLE_AUTOLOAD=1
define gdox
$(GENDLOPEN) -i $(1).txt -n $(2) --default-library $(3) $(4) \
	-f -o example_$(1).h
endef


.PHONY: all clean run_tests run_tests2

all: $(BINS)

test_all: $(BINS) $(BINS2)

clean:
	-rm -f $(BINS) notify appindicator uuid_nix
	-rm -f *.exe *.dll *.dylib *.so *.so.* *.lib *.a *.o *.obj
	-rm -f example_*.h example_*.hpp
	-rm -f uuid_*_dl.h example_helloworld_static_dl.c

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

# test different ways to add an include header line
example_helloworld_static_dl.c: helloworld.txt
	$(GENDLOPEN) -i $^ -f -o $@ \
		-I"<helloworld.h>" -I"helloworld.h" -I'"helloworld.h"' -I"'helloworld.h'" \
		-DBUILDING_STATIC -DGDO_ENABLE_AUTOLOAD=1 -DGDO_WRAP_FUNCTIONS=1 \
		--default-library="helloworld:0"

# a static library replacement for libhelloworld.so
example_helloworld_static_dl.$(OBJEXT): example_helloworld_static_dl.c
	$(call compile_cc,$<,-c)

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
	$(call compile_cc,$<,,$(LIBX))

helloworld_example_dl: libhelloworld_dl.$(LIBEXT)
	$(call compile_cc,helloworld_example.c,-DBUILDING_STATIC,libhelloworld_dl.$(LIBEXT))

# test filename with hyphens and "--skip-parameter-names"
appindicator: appindicator.c
	$(call gdox,appindicator,xappindicator,'"libappindicator.so.1"',--skip-parameter-names)
	$(call gdox,appindicator_gobject,xgobject,libgobject-2.0.so.0,)
	$(call gdox,appindicator_gtk,xgtk,libgtk-x11-2.0.so.0,)
	$(call compile_cc, $<, -DUSE_DLOPEN=1 `pkg-config --cflags appindicator-0.1`)

# test filename with colon
notify: notify.c
	$(call gdox,notify,xnotify,notify:4,)
	$(call gdox,notify_gobject,xgobject,gobject-2.0:0,)
	$(call compile_cc, $<, -DUSE_DLOPEN=1 `pkg-config --cflags libnotify`)

uuid_nix: uuid_nix.c
	$(CLANG) -Xclang -ast-dump -I/usr/include/uuid $@.h | \
	  $(PARSE_CLANG_AST) list uuid_generate_random,uuid_is_null,uuid_unparse_lower | \
	  $(GENDLOPEN) -i- --default-library libuuid.so.1 -f -o $@_dl.h
	$(call compile_cc,$<,-I/usr/include/uuid)

uuid_win: uuid_win.c
	$(CLANG) -Xclang -ast-dump -fansi-escape-codes $@.h | \
	  $(PARSE_CLANG_AST) list UuidCreate,UuidToStringA,RpcStringFreeA | \
	  $(GENDLOPEN) -i- --default-library rpcrt4.dll -f -o $@_dl.h
	$(call compile_cc,$<)

example_%.h: helloworld.txt
	$(GENDLOPEN) -i $^ -f -o $@

example_%.hpp: helloworld.txt
	$(GENDLOPEN) --format=C++ -i $^ -f -o $@

