ifneq ($(CLANG_CL),)
STDCXX  := c++latest
else
STDCXX  := c++23
endif

include ../include.mk


BIN     := gendlopen
AUXBIN  := gen_template_h
OBJS    := generate.$(OBJEXT) main.$(OBJEXT) parse.$(OBJEXT) tokenize.$(OBJEXT)
SRCS    := generate.cpp main.cpp parse.cpp tokenize.cpp
GENHDRS := template.h
HDRS    := $(GENHDRS) args.hxx common.hpp gendlopen.hpp tokenize.hpp


.PHONY: all clean

all: $(BIN)

clean:
	-rm -f $(BIN) $(AUXBIN) $(GENHDRS) *.exe *.o *.obj

$(SRCS): $(HDRS)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(AUXBIN): gen_template_h.c
	$(CCAUX) -o $@ $<

template.h: $(AUXBIN)
	./$(AUXBIN) templates
