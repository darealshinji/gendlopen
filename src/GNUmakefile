ifneq ($(CLANG_CL),)
STDCXX  := c++latest
else
STDCXX  := c++20
endif

include ../include.mk


BIN    := gendlopen
AUXBIN := gen_template_h

OBJS   := main.$(OBJEXT) \
	clang-ast.$(OBJEXT) \
	generate.$(OBJEXT) \
	parse.$(OBJEXT) \
	tokenize.$(OBJEXT) \
	$(NULL)


.PHONY: all clean

all: $(BIN)

clean:
	-rm -f $(BIN) $(AUXBIN) template.h *.exe *.o *.obj

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(AUXBIN): gen_template_h.c
	$(CCAUX) -Wall -O2 -o $@ $<

generate.cpp: template.h

template.h: $(AUXBIN)
	./$(AUXBIN) ../templates
