!IF !DEFINED( V )
V = 0
!ENDIF

!IF "$(V)" == "0" || "$(V)" == ""
silence = 2>NUL
.SILENT:
!ENDIF

!IF !DEFINED( CLANG )
CLANG = clang
!ENDIF

!IF [$(CLANG) --version 2>NUL >NUL] == 0
uuid_win = uuid_win.exe
!ELSE
!MESSAGE warning: clang not found
!ENDIF

CC       = cl -nologo
CXX      = cl -nologo
LIB_EXE  = lib -nologo
CPPFLAGS = -DWIN32_LEAN_AND_MEAN
CFLAGS   = -W3 -O2 -I.
CXXFLAGS = -W3 -O2 -I. -std:c++latest -EHsc
LFLAGS   = -subsystem:console -release

JOBS = 4

# top source directory
s = ..

test_bins = \
	c_test.exe \
	c_auto_free.exe \
	c_autoload.exe \
	c_minimal.exe \
	c_win32_wide.exe \
	c_multi.exe \
	cxx_test.exe \
	cxx_autoload.exe \
	cxx_minimal.exe \
	cxx_win32_wide.exe \
	cxx_multi.exe \
	helloworld_example_dl.exe \
	$(uuid_win)

cleanfiles_check = \
	$(test_bins) \
	c_*.obj \
	cxx_*.obj \
	c_*.h \
	cxx_*.hpp \
	c_multi.c \
	cxx_multi.cpp \
	uuid_win_dl.h \
	libhelloworld-0.* \
	libhelloworld_dl.*

templ_objs = \
	templ_filename_macros.h.obj \
	templ_license.h.obj \
	templ_common.h.obj \
	templ_c.h.obj \
	templ_c.c.obj \
	templ_cxx.hpp.obj \
	templ_cxx.cpp.obj \
	templ_minimal.h.obj \
	templ_minimal_cxxeh.hpp.obj

sources = \
	$(s)\src\getopt\getopt.c \
	$(s)\src\cio_ifstream.cpp \
	$(s)\src\cio_ofstream.cpp \
	$(s)\src\clang-ast.cpp \
	$(s)\src\generate.cpp \
	$(s)\src\getopt_long_cxx.cpp \
	$(s)\src\main.cpp \
	$(s)\src\parse.cpp \
	$(s)\src\tokenize.cpp \
	$(s)\src\utils.cpp


gen_hdr  = gendlopen.exe -i $(s)\test\helloworld.txt -f -o
cc_test  = $(CC) $(CFLAGS) $(CPPFLAGS) -I$(s)\test $(s)\test\$*.c -Fe$@ -link $(LFLAGS)
cxx_test = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(s)\test $(s)\test\$*.cpp -Fe$@ -link $(LFLAGS)



all: gendlopen.exe

clean-check:
	-del /q $(cleanfiles_check) $(silence)

clean: clean-check
	-del /q *.exe *.obj *.lib $(silence)

check: $(test_bins)

run_tests: $(test_bins)
	for %%x in ( $(test_bins) ) do %%x

$(test_bins): gendlopen.exe libhelloworld-0.dll


## main program ##

$(templ_objs): file_to_obj.exe
	$**

file_to_obj.exe: $(s)\src\file_to_obj.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DCUSTOM_MAIN $** -Fe$@ -link $(LFLAGS)

gendlopen.exe: $(sources) $(templ_objs)
	$(CXX) -MP$(JOBS) $(CXXFLAGS) $(CPPFLAGS) -I$(s)\src\getopt $(sources) -Fe$@ \
	  -link $(LFLAGS) $(templ_objs)


## dll ##

libhelloworld-0.dll:
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(s)\test -DBUILDING_DLL -LD $(s)\test\helloworld.c -Fe$@


## C tests ##

c_test.exe:
	$(gen_hdr) $*.h -DGDO_WRAP_FUNCTIONS && $(cc_test)

c_auto_free.exe:
	$(gen_hdr) $*.h --skip-param && $(cc_test)

c_autoload.exe:
	$(gen_hdr) $*.h && $(cc_test)

c_minimal.exe:
	$(gen_hdr) $*.h --format=minimal && $(cc_test)

c_win32_wide.exe:
	$(gen_hdr) $*.h && $(CC) $(CFLAGS) $(CPPFLAGS) -D_UNICODE -DUNICODE \
	  -I$(s)\test $(s)\test\$*.c -Fe$@ -link $(LFLAGS)

c_multi.exe:
	$(gen_hdr) $*.h --separate --library=API:0:helloworld --include=helloworld.h \
	  -DGDO_WRAP_FUNCTIONS -DGDO_ENABLE_AUTOLOAD -DBUILDING_STATIC && \
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_UNICODE -DUNICODE -I$(s)\test \
	  $*.c $(s)\test\c_multi_part1.c $(s)\test\c_multi_part2.c -Fe$@ -link $(LFLAGS)


## C++ tests ##

cxx_test.exe:
	$(gen_hdr) $*.hpp --format=C++ --skip-param && \
	  $(cxx_test)

cxx_autoload.exe:
	$(gen_hdr) $*.hpp --format=C++ --library=API:0:helloworld && \
	  $(cxx_test)

cxx_minimal.exe:
	$(gen_hdr) $*.hpp --format=minimal-C++ && \
	  $(cxx_test)

cxx_win32_wide.exe:
	$(gen_hdr) $*.hpp --format=C++ && \
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -D_UNICODE -DUNICODE -I$(s)\test \
	  $(s)\test\$*.cpp -Fe$@ -link $(LFLAGS)

cxx_multi.exe:
	$(gen_hdr) $*.hpp --format=C++ --separate --library=API:0:helloworld \
	  --include=helloworld.h -DGDO_ENABLE_AUTOLOAD -DBUILDING_STATIC && \
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -D_UNICODE -DUNICODE -I$(s)\test \
	  $*.cpp $(s)\test\cxx_multi_part1.cpp $(s)\test\cxx_multi_part2.cpp -Fe$@ -link $(LFLAGS)


# static fake library test

libhelloworld_dl.obj:
	$(gen_hdr) $*.h --separate --library=API:0:helloworld --include=helloworld.h \
	  -DBUILDING_STATIC -DGDO_ENABLE_AUTOLOAD && \
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(s)\test -c $*.c -Fe$@

libhelloworld_dl.lib: libhelloworld_dl.obj
	$(LIB_EXE) -out:$@ libhelloworld_dl.obj

helloworld_example_dl.exe: libhelloworld_dl.lib
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(s)\test -DBUILDING_STATIC \
	  $(s)\test\helloworld_example.c -Fe$@ libhelloworld_dl.lib -link $(LFLAGS)


# UUID example
!IFDEF CLANG

uuid_win.exe:
	$(CLANG) -Xclang -ast-dump $(s)\examples\uuid_win.h | \
	  gendlopen.exe -i- -f -o uuid_win_dl.h --library rpcrt4.dll \
	  -SUuidCreate -SUuidToStringA -SRpcStringFreeA && \
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(s)\examples $(s)\examples\$*.c -Fe$@ -link $(LFLAGS)

!ENDIF
