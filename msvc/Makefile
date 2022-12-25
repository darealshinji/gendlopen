# Makefile for Microsoft nmake.exe
cc       = cl -nologo
link     = link -nologo -release
ggo      = gengetopt.exe
cflags   = -W3 -O2 -I.
cxxflags = -W3 -O2 -EHsc -I.
objs     = gendlopen.obj getopt.obj cmdline.obj main.obj
genhdrs  = c_header.h cxx_header.h lib_macros.h


all: gendlopen.exe

clean:
	-del /q gendlopen.exe $(objs)
	-del /q xxd.exe xxd.obj $(genhdrs)

gendlopen.exe: $(objs)
	$(link) $(ldebug) $(conflags) -out:gendlopen.exe $(objs) $(conlibs) rpcrt4.lib

gendlopen.obj: $(genhdrs)
	$(cc) $(cdebug) $(cxxflags) $(cvars) -c ..\gendlopen.cpp

getopt.obj: getopt.c getopt.h
	$(cc) $(cdebug) $(cflags) $(cvars) -c getopt.c

cmdline.obj: ..\cmdline.h ..\cmdline.c
	$(cc) $(cdebug) -D_CRT_SECURE_NO_WARNINGS $(cflags) $(cvars) -c ..\cmdline.c

main.obj: ..\cmdline.h
	$(cc) $(cdebug) $(cxxflags) $(cvars) -c ..\main.cpp

..\cmdline.h:
	cd .. && $(ggo) < cmdline.ggo

xxd.exe:
	$(cc) $(cdebug) $(cflags) $(cvars) xxd.c $(conlibs)

c_header.h: xxd.exe ..\c_header.template
	xxd.exe < ..\$*.template > $*.h

cxx_header.h: xxd.exe ..\cxx_header.template
	xxd.exe < ..\$*.template > $*.h

lib_macros.h: xxd.exe ..\lib_macros.template
	xxd.exe < ..\$*.template > $*.h

