OUT = out

OBJS = \
	$(OUT)/cio_ofstream.o \
	$(OUT)/clang-ast.o \
	$(OUT)/data.o \
	$(OUT)/gendlopen.o \
	$(OUT)/generate.o \
	$(OUT)/help.o \
	$(OUT)/lex.yy.o \
	$(OUT)/main.o \
	$(OUT)/open_file.o \
	$(OUT)/parameters.o \
	$(OUT)/parse_args.o \
	$(OUT)/parse.o \
	$(OUT)/parse_options.o \
	$(OUT)/substitute.o \
	$(OUT)/tokenize.o \
	$(OUT)/utils.o

CFLAGS      = -Wall -O3
CXXFLAGS    = -Wall -O3 -std=c++20 -I$(OUT)
LDFLAGS     = -Wl,-O1 -s
COMPILE_C   = $(CC) $(CFLAGS) $(CPPFLAGS) -c -o
COMPILE_CXX = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o



all: $(OUT)/gendlopen

clean:
	-rm -rf $(OUT)

$(OUT)/gendlopen: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OUT)/.gitignore:
	mkdir -p $(OUT) && echo '*' > $@

$(OBJS): $(OUT)/.gitignore

$(OUT)/template.h: $(OUT)/gen_template_h
	$(OUT)/gen_template_h templates $(OUT)/template.h

$(OUT)/gen_template_h: $(OUT)/.gitignore gen_template_h.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ gen_template_h.c $(LDFLAGS)

data.cpp: $(OUT)/template.h

$(OUT)/cio_ofstream.o: cio_ofstream.cpp
	$(COMPILE_CXX) $@ cio_ofstream.cpp

$(OUT)/clang-ast.o: clang-ast.cpp
	$(COMPILE_CXX) $@ clang-ast.cpp

$(OUT)/data.o: data.cpp
	$(COMPILE_CXX) $@ data.cpp

$(OUT)/gendlopen.o: gendlopen.cpp
	$(COMPILE_CXX) $@ gendlopen.cpp

$(OUT)/generate.o: generate.cpp
	$(COMPILE_CXX) $@ generate.cpp

$(OUT)/help.o: help.cpp
	$(COMPILE_CXX) $@ help.cpp

$(OUT)/lex.yy.o: lex.yy.c
	$(COMPILE_C) $@ lex.yy.c

$(OUT)/main.o: main.cpp
	$(COMPILE_CXX) $@ main.cpp

$(OUT)/open_file.o: open_file.cpp
	$(COMPILE_CXX) $@ open_file.cpp

$(OUT)/parameters.o: parameters.cpp
	$(COMPILE_CXX) $@ parameters.cpp

$(OUT)/parse_args.o: parse_args.cpp
	$(COMPILE_CXX) $@ parse_args.cpp

$(OUT)/parse.o: parse.cpp
	$(COMPILE_CXX) $@ parse.cpp

$(OUT)/parse_options.o: parse_options.cpp
	$(COMPILE_CXX) $@ parse_options.cpp

$(OUT)/substitute.o: substitute.cpp
	$(COMPILE_CXX) $@ substitute.cpp

$(OUT)/tokenize.o: tokenize.cpp
	$(COMPILE_CXX) $@ tokenize.cpp

$(OUT)/utils.o: utils.cpp
	$(COMPILE_CXX) $@ utils.cpp

