OUT = out
SRC = src

CXX_SRCS = \
	..\$(SRC)\cio_ofstream.cpp \
	..\$(SRC)\clang-ast.cpp \
	..\$(SRC)\data.cpp \
	..\$(SRC)\filesystem_compat.cpp \
	..\$(SRC)\gendlopen.cpp \
	..\$(SRC)\generate.cpp \
	..\$(SRC)\help.cpp \
	..\$(SRC)\main.cpp \
	..\$(SRC)\open_file.cpp \
	..\$(SRC)\parse.cpp \
	..\$(SRC)\parse_args.cpp \
	..\$(SRC)\parse_options.cpp \
	..\$(SRC)\parse_parameters.cpp \
	..\$(SRC)\substitute.cpp \
	..\$(SRC)\symbol_name_lookup.cpp \
	..\$(SRC)\tokenize.cpp \
	..\$(SRC)\utils.cpp

CFLAGS   = /W3 /O2 /I..\$(SRC)
CXXFLAGS = /W3 /O2 /I..\$(SRC) /I. /EHsc /std:c++20



all: $(OUT)\gendlopen.exe

clean:
	-rmdir /Q /S $(OUT)

$(OUT)\gendlopen.exe: $(OUT)\lex.yy.obj $(OUT)\template.h
	cd $(OUT)\ && $(CXX) /nologo /MP $(CXXFLAGS) $(CXX_SRCS) /Fe:gendlopen.exe /link lex.yy.obj $(LFLAGS)

# .dirstamp
$(OUT)\.gitignore:
	mkdir $(OUT) && echo * > $@

$(OUT)\lex.yy.obj: $(OUT)\.gitignore
	cd $(OUT)\ && $(CC) /nologo $(CFLAGS) /c ..\$(SRC)\lex.yy.c

$(OUT)\template.h: $(OUT)\gen_template_h.exe
	$(OUT)\gen_template_h.exe $(SRC)\templates $(OUT)\template.h

$(OUT)\gen_template_h.exe: $(OUT)\.gitignore
	cd $(OUT)\ && $(CC) /nologo /Fe:gen_template_h.exe ..\$(SRC)\gen_template_h.c

