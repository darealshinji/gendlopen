bins = \
	example_1.exe \
	example_autoload.exe \
	example_auto_free.exe \
	example_cxx.exe \
	example_cxx_autoload.exe \
	example_wide.exe \
	example_cxx_wide.exe \
	helloworld_example_nodl.exe \
	helloworld_example_dl.exe

bins2 = uuid_win.exe

genhdrs_h = \
	example_1.h \
	example_autoload.h \
	example_auto_free.h \
	example_wide.h \
	example_helloworld_static_dl.h

genhdrs_hpp = \
	example_cxx.hpp \
	example_cxx_autoload.hpp \
	example_cxx_wide.hpp

dll = libhelloworld-0.dll

cl       = cl -nologo
clang    = clang
link     = link -nologo
lib_exe  = lib -nologo
cflags   = -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN
cxxflags = -W3 -O2 -I. -DWIN32_LEAN_AND_MEAN -EHsc -std:c++latest
ldflags  = -subsystem:console -release

gendlopen       = ..\src\gendlopen.exe
parse_clang_ast = ..\src\parse-clang-ast.exe


all: $(bins) $(bins2)

clean:
	-del /q *.exe *.dll *.dylib *.so *.so.* *.lib *.a *.o *.obj *.exp
	-del /q $(genhdrs_h) $(genhdrs_hpp) uuid_*_dl.h example_helloworld_static_dl.c

$(bins): $(dll)

libhelloworld.lib: $(dll)

$(dll): helloworld.c
	$(cl) $(cflags) -DBUILDING_DLL $** -link -dll -out:$(dll) -implib:libhelloworld.lib $(ldflags)

libhelloworld_dl.lib: example_helloworld_static_dl.obj
	$(lib_exe) -out:$@ $**

# test different ways to add an include header line
example_helloworld_static_dl.c: helloworld.txt
	$(gendlopen) -i $** -f -o $@ \
		-I"<helloworld.h>" -I"QUOTE_STRING(helloworld.h)" \
		-DBUILDING_STATIC -DGDO_ENABLE_AUTOLOAD=1 -DGDO_WRAP_FUNCTIONS=1 \
		--default-library="LIBNAME(helloworld,0)"

example_helloworld_static_dl.obj: example_helloworld_static_dl.c
	$(cl) $(cflags) -c $*.c

helloworld_example_dl.exe: libhelloworld_dl.lib
	$(cl) $(cflags) -DBUILDING_STATIC helloworld_example.c -link -out:$@ libhelloworld_dl.lib $(ldflags)

helloworld_example_nodl.exe: libhelloworld.lib
	$(cl) $(cflags) helloworld_example.c -link -out:$@ libhelloworld.lib $(ldflags)

example_1.exe: example_1.h
	$(cl) $(cflags) $*.c -link -out:$@ $(ldflags)

example_auto_free.exe: example_auto_free.h
	$(cl) $(cflags) $*.c -link -out:$@ $(ldflags)

example_autoload.exe: example_autoload.h
	$(cl) $(cflags) $*.c -link -out:$@ $(ldflags)

example_cxx.exe: example_cxx.hpp
	$(cl) $(cxxflags) $*.cpp -link -out:$@ $(ldflags)

example_cxx_autoload.exe: example_cxx_autoload.hpp
	$(cl) $(cxxflags) $*.cpp -link -out:$@ $(ldflags)

example_wide.exe: example_wide.h
	$(cl) $(cflags) -DUNICODE -D_UNICODE $*.c -link -out:$@ $(ldflags)

example_cxx_wide.exe: example_cxx_wide.hpp
	$(cl) $(cxxflags) $*.cpp -link -out:$@ $(ldflags)

uuid_win_dl.h:
	$(clang) -Xclang -ast-dump -fansi-escape-codes uuid_win.h | \
	  $(parse_clang_ast) list UuidCreate,UuidToStringA,RpcStringFreeA | \
	  $(gendlopen) -i- --default-library "QUOTE_STRING(rpcrt4.dll)" -f -o $@

uuid_win.exe: uuid_win_dl.h
	$(cl) $(cflags) $*.c -link -out:$@ $(ldflags)

$(genhdrs_h): helloworld.txt
	$(gendlopen) -i $** -f -o $@

$(genhdrs_hpp): helloworld.txt
	$(gendlopen) --format=C++ -i $** -f -o $@
