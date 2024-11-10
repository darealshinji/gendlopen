%{
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int token_count = 0;
static char errbuf[128] = {0};
static void illegal_char();

#define MYLEX_ERROR     -1
#define MYLEX_OK         1
#define MYLEX_CLANG_AST  2
%}

comment1        "//".*
comment2        [/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]

color           [\x1B][\[][0-9;]*[m]
ast             ^"TranslationUnitDecl 0x".*
ast_color       ^{color}"TranslationUnitDecl"{color}{color}" 0x".*

option          ^"%option"[ \t].*
utf8_bom        ^"\xEF\xBB\xBF"

identifier      [a-zA-Z_][a-zA-Z0-9_]*
triple_dot      "..."
other           [*,()\[\]]
semicolon       ;
illegal         .

%option noinput nounput noyywrap yylineno

%%
[[:space:]]         ;

{utf8_bom}          { /* ignore BOM at beginning of first line */
    if (yylineno != 1) {
        illegal_char();
        return MYLEX_ERROR;
    }
}

{comment1}          ;
{comment2}          ;

{ast}               { /* clang AST identifier in first line */
    if (yylineno == 1) {
        return MYLEX_CLANG_AST;
    }
}

{ast_color}         {
    if (yylineno == 1) {
        return MYLEX_CLANG_AST;
    }
}

{option}            { token_count = 0; return MYLEX_OK; }
{identifier}        { token_count++; return MYLEX_OK; }
{triple_dot}        { token_count++; return MYLEX_OK; }
{other}             { token_count++; return MYLEX_OK; }

{semicolon}         { /* return if statement was not empty */
    if (token_count > 0) {
        token_count = 0;
        return MYLEX_OK;
    }
}

{illegal}           { illegal_char(); return MYLEX_ERROR; }
%%

/* save error message */
static void illegal_char()
{
    const char *fmt = "illegal character `%c' at line %d";
    const size_t buflen = sizeof(errbuf) - 1;

    if (!isprint(*yytext)) {
        fmt = "illegal character `\\%03u' at line %d";
    }

#ifdef _WIN32
    _snprintf_s(errbuf, sizeof(errbuf), buflen, fmt, *yytext, yylineno);
#else
    snprintf(errbuf, buflen, fmt, *yytext, yylineno);
#endif

    errbuf[buflen] = 0;
}

const char *mylex_lasterror() {
    return (errbuf[0] == 0) ? NULL : errbuf;
}

int mylex(FILE *fp)
{
    if (fp && yyget_in() != fp) {
        yyset_in(fp);
    }

    errbuf[0] = 0;

    return yylex();
}