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
    LEX_STRUCT,
    LEX_UNION,
    LEX_ENUM,
    LEX_TYPEDEF,
    LEX_ID,
    LEX_OTHER,
    LEX_CURLY_OPEN,
    LEX_CURLY_CLOSE,
    LEX_EQUAL,
    LEX_SEMICOLON
};

extern const char *lex_errmsg;
extern char *yytext;

void set_illegal_char();

int yylex();
void yyset_in(FILE *);


#ifdef __cplusplus
} /* extern "C" */

namespace lex
{
    inline const char *error() {
        return (::lex_errmsg[0] == 0) ? NULL : ::lex_errmsg;
    }

    inline const char *text() {
        return ::yytext;
    }

    inline void set_illegal_char() {
        ::set_illegal_char();
    }

    inline int lex() {
        return ::yylex();
    }

    inline void set_fpin(FILE *fp) {
        ::yyset_in(fp);
    }
}
#endif //__cplusplus
