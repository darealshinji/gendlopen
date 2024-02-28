# NMake Makefile

cl       = cl -nologo
link     = link -nologo
cflags   = -W3 -O2 -DWIN32_LEAN_AND_MEAN
cxxflags = $(cflags) -EHsc -std:c++latest
ldflags  = -subsystem:console -release
objs     = generate.obj main.obj parse.obj tokenize.obj


all: gendlopen.exe parse-clang-ast.exe

clean:
	-del /q *.exe *.obj template.h

template.h: gen_template_h.exe
	$** templates

gen_template_h.exe: gen_template_h.c
	$(cl) $(cflags) $** -link -out:$@ $(ldflags)

gendlopen.exe: $(objs)
	$(link) -out:$@ $** $(ldflags)

parse-clang-ast.exe: parse-clang-ast.cpp
	$(cl) $(cxxflags) -DNDEBUG $** -link -out:$@ $(ldflags)

$(objs): template.h

.cpp.obj:
	$(cl) $(cxxflags) -c $*.cpp
