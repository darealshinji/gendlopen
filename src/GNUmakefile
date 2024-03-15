ifneq ($(CLANG_CL),)
STDCXX  := c++latest
else
STDCXX  := c++20
endif

include ../include.mk


BIN     := gendlopen
BIN2    := parse-clang-ast
AUXBIN  := gen_template_h
OBJS    := generate.$(OBJEXT) main.$(OBJEXT) parse.$(OBJEXT) tokenize.$(OBJEXT)
SRCS    := generate.cpp main.cpp parse.cpp tokenize.cpp
HDRS    := args.hxx common.hpp gendlopen.hpp tokenize.hpp parse-clang-ast.hpp template.h


.PHONY: all clean

all: $(BIN) $(BIN2)

clean:
	-rm -f $(BIN) $(BIN2) $(AUXBIN) template.h *.exe *.o *.obj

$(SRCS): $(HDRS)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN2): parse-clang-ast.cpp
	$(call compile_cxx,$^)

$(AUXBIN): gen_template_h.c
	$(CCAUX) -Wall -O2 -o $@ $<

template.h: $(AUXBIN)
	./$(AUXBIN) ../templates
