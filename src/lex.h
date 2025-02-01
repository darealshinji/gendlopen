#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum {
    LEX_ERROR = -2,
    LEX_AST_BEGIN = 258,
    LEX_AST_FUNCVAR,
    LEX_AST_PARMVAR,
    LEX_OPTION,
    LEX_ID,
    LEX_OTHER,
    LEX_SEMICOLON
};

extern char *yytext;

int lex(FILE *fp);
const char *lex_lasterr();

#ifdef __cplusplus
}
#endif
