OUT = out
SRC = src

OBJS = \
	$(OUT)/cio_ofstream.o \
	$(OUT)/clang-ast.o \
	$(OUT)/data.o \
	$(OUT)/filesystem_compat.o \
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
	$(OUT)/symbol_name_lookup.o \
	$(OUT)/tokenize.o \
	$(OUT)/utils.o

CFLAGS      = -Wall -O3 -I$(SRC)
CXXFLAGS    = -Wall -O3 -I$(SRC) -I$(OUT) -std=c++20
LDFLAGS     = -Wl,-O1 -s
COMPILE_C   = $(CC) $(CFLAGS) $(CPPFLAGS) -c -o
COMPILE_CXX = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o



all: $(OUT)/gendlopen

clean:
	-rm -rf $(OUT)

$(OUT)/gendlopen: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

# .dirstamp
$(OUT)/.gitignore:
	mkdir -p $(OUT) && echo '*' > $@

$(OBJS): $(OUT)/.gitignore

$(OUT)/template.h: $(OUT)/gen_template_h
	$(OUT)/gen_template_h $(SRC)/templates $(OUT)/template.h

$(OUT)/gen_template_h: $(OUT)/.gitignore $(SRC)/gen_template_h.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRC)/gen_template_h.c $(LDFLAGS)

$(SRC)/data.cpp: $(OUT)/template.h

$(OUT)/cio_ofstream.o: $(SRC)/cio_ofstream.cpp
	$(COMPILE_CXX) $@ $(SRC)/cio_ofstream.cpp

$(OUT)/clang-ast.o: $(SRC)/clang-ast.cpp
	$(COMPILE_CXX) $@ $(SRC)/clang-ast.cpp

$(OUT)/data.o: $(SRC)/data.cpp
	$(COMPILE_CXX) $@ $(SRC)/data.cpp

$(OUT)/filesystem_compat.o: $(SRC)/filesystem_compat.cpp
	$(COMPILE_CXX) $@ $(SRC)/filesystem_compat.cpp

$(OUT)/gendlopen.o: $(SRC)/gendlopen.cpp
	$(COMPILE_CXX) $@ $(SRC)/gendlopen.cpp

$(OUT)/generate.o: $(SRC)/generate.cpp
	$(COMPILE_CXX) $@ $(SRC)/generate.cpp

$(OUT)/help.o: $(SRC)/help.cpp
	$(COMPILE_CXX) $@ $(SRC)/help.cpp

$(OUT)/lex.yy.o: $(SRC)/lex.yy.c
	$(COMPILE_C) $@ $(SRC)/lex.yy.c

$(OUT)/main.o: $(SRC)/main.cpp
	$(COMPILE_CXX) $@ $(SRC)/main.cpp

$(OUT)/open_file.o: $(SRC)/open_file.cpp
	$(COMPILE_CXX) $@ $(SRC)/open_file.cpp

$(OUT)/parameters.o: $(SRC)/parameters.cpp
	$(COMPILE_CXX) $@ $(SRC)/parameters.cpp

$(OUT)/parse_args.o: $(SRC)/parse_args.cpp
	$(COMPILE_CXX) $@ $(SRC)/parse_args.cpp

$(OUT)/parse.o: $(SRC)/parse.cpp
	$(COMPILE_CXX) $@ $(SRC)/parse.cpp

$(OUT)/parse_options.o: $(SRC)/parse_options.cpp
	$(COMPILE_CXX) $@ $(SRC)/parse_options.cpp

$(OUT)/substitute.o: $(SRC)/substitute.cpp
	$(COMPILE_CXX) $@ $(SRC)/substitute.cpp

$(OUT)/symbol_name_lookup.o: $(SRC)/symbol_name_lookup.cpp
	$(COMPILE_CXX) $@ $(SRC)/symbol_name_lookup.cpp

$(OUT)/tokenize.o: $(SRC)/tokenize.cpp
	$(COMPILE_CXX) $@ $(SRC)/tokenize.cpp

$(OUT)/utils.o: $(SRC)/utils.cpp
	$(COMPILE_CXX) $@ $(SRC)/utils.cpp

