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

static int count = 0;
static int lineno = 1;
static char errbuf[128] = {0};

int mylex(FILE *fp);
const char *mylex_lasterror();
%}

newline         [\n]
comment1        "//".*
comment2        [/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]
white_space     [ \t\r]+
identifier      [a-zA-Z_][a-zA-Z0-9_]*
triple_dot      "..."
other           [*,()\[\]]
semicolon       ;
illegal         .

%option noyywrap noinput nounput

%%
{newline}           lineno++;
{white_space}       /* do nothing */
{comment1}          /* do nothing */

{comment2} { /* count lines */
    for (char *p = yytext; *p != 0; p++) {
        if (*p == '\n') {
            lineno++;
        }
    }
}

{identifier}        count++; return 1;
{triple_dot}        count++; return 1;
{other}             count++; return 1;

{semicolon} { /* return if statement was not empty */
    if (count > 0) {
        count = 0;
        return 1;
    }
}

{illegal} { /* save error message */
    const char *fmt = "illegal character `%c' at line %d";
    const size_t count = sizeof(errbuf) - 1;

    if (!isprint(*yytext)) {
        fmt = "illegal character `\\%03u' at line %d";
    }

#ifdef _WIN32
    _snprintf_s(errbuf, sizeof(errbuf), count, fmt, *yytext, lineno);
#else
    snprintf(errbuf, count, fmt, *yytext, lineno);
#endif
    errbuf[count] = 0;

    return -1;
}
%%

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
