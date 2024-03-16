ifneq ($(CLANG_CL),)
STDCXX  := c++latest
else
STDCXX  := c++20
endif

include ../include.mk


BIN    := gendlopen
AUXBIN := gen_template_h

SRCS := main.cpp generate.cpp parse.cpp parse-clang-ast.cpp tokenize.cpp
HDRS := args.hxx common.hpp gendlopen.hpp tokenize.hpp template.h

OBJS := main.$(OBJEXT) \
	generate.$(OBJEXT) \
	parse.$(OBJEXT) \
	parse-clang-ast.$(OBJEXT) \
	tokenize.$(OBJEXT) \
	$(NULL)


.PHONY: all clean

all: $(BIN)

clean:
	-rm -f $(BIN) $(AUXBIN) template.h *.exe *.o *.obj

$(SRCS): $(HDRS)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(AUXBIN): gen_template_h.c
	$(CCAUX) -Wall -O2 -o $@ $<

template.h: $(AUXBIN)
	./$(AUXBIN) ../templates
