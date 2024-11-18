#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>

#include "types.hpp"
#include "utils.hpp"
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "parse_args.hpp"
#include "gendlopen.hpp"

/* lex.yy.c */
enum {
    //MYLEX_EOF = 0,
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_OPTION,
    MYLEX_TOKEN,
    MYLEX_SEMICOLON,
    MYLEX_ERROR
};
extern "C" char *yytext;
extern "C" int mylex(FILE *fp);
extern "C" const char *mylex_lasterror();

/* help.cpp */
namespace help {
    void print(const char *prog);
    void print_full(const char *prog);
}

#endif /* GLOBAL_HPP */
