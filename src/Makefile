# This one Makefile works with Microsoft nmake and GNU make.
# They use different conditional syntax, but each can be
# nested and inverted within the other.

all: default


ifdef MAKEDIR: # gmake: false; nmake: unused target
!ifdef MAKEDIR # gmake: not seen; nmake: true

#
# Microsoft nmake
#

!include ..\nmake.mk


default: gendlopen.exe

clean:
	-del /q *.exe *.obj template.h

template.h: gen_template_h.exe
	$** templates

gen_template_h.exe: gen_template_h.c
	$(CL) $(CFLAGS) $** -Fe$@ $(LDFLAGS)

gendlopen.exe: main.cpp clang-ast.cpp generate.cpp parse.cpp tokenize.cpp utils.cpp
	$(CL) $(CXXFLAGS) $** -Fe$@ $(LDFLAGS)

generate.cpp: template.h

.cpp.obj:
	$(CL) $(CXXFLAGS) -c $*.cpp


!else # and now the other
else


#
# GNU make
#

STDCXX ?= c++20

include ../gmake.mk


CCAUX  ?= $(CC)

BIN     = gendlopen
AUXBIN  = gen_template_h

OBJS = main.o clang-ast.o generate.o parse.o tokenize.o utils.o


.PHONY: all default clean


default: $(BIN)

clean:
	-rm -f $(BIN) $(AUXBIN) template.h *.exe *.o *.obj

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(AUXBIN): gen_template_h.c
	$(CCAUX) -Wall -O2 -o $@ $<

generate.cpp: template.h

template.h: $(AUXBIN)
	./$(AUXBIN) templates


endif    # gmake: close condition; nmake: not seen
!endif : # gmake: unused target; nmake close conditional

