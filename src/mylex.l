%{

/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#include <ctype.h>

static int tokens_read = 0;
static char errbuf[128] = {0};
static void illegal_char();

enum {
    //MYLEX_EOF = 0,
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_AST_PARMVAR,
    MYLEX_OPTION,
    MYLEX_TOKEN,
    MYLEX_SEMICOLON,
    MYLEX_ERROR
};
%}

bom                  "\xEF\xBB\xBF"
col                  [\x1B][\[][0-9;]*[m]
linewrap             "\\"[[:space:]]*?"\n"

ast_begin            ^({bom})?({col})?"TranslationUnitDecl"({col})?({col})?" 0x".*
funcvar              ^({col})?[|`]"-"({col})?({col})?("Function"|"Var")"Decl".*
parmvar              ^({col})?[| ]" "[|`]"-"({col})?({col})?"ParmVarDecl".*

option               ^({bom})?"%option"[[:space:]]
optstr               [^[:space:]]+

preproc              ^({bom})?[[:space:]]*?"#"

identifier           [a-zA-Z0-9_]+
other                [*,()\[\]]

%option noinput nounput noyywrap yylineno nounistd never-interactive

%x CLANG_AST
%x C_COMMENT
%x LN_COMMENT
%x PREPROC
%x OPTION

%%

{ast_begin}          { /* clang AST identifier in first line */
    if (yylineno == 1) {
        BEGIN(CLANG_AST);
        return MYLEX_AST_BEGIN;
    }
}

<CLANG_AST>{funcvar} { return MYLEX_OK; }
<CLANG_AST>{parmvar} { return MYLEX_AST_PARMVAR; }
<CLANG_AST>\n        { }
<CLANG_AST>.         { }

"/*"                 { BEGIN(C_COMMENT); }
<C_COMMENT>"*/"      { BEGIN(INITIAL); }
<C_COMMENT>\n        { }
<C_COMMENT>.         { }

"//"                 { BEGIN(LN_COMMENT); }
<LN_COMMENT>\n       { BEGIN(INITIAL); }
<LN_COMMENT>.        { }

{preproc}            { BEGIN(PREPROC); }
<PREPROC>{linewrap}  { /* continue to next line */ }
<PREPROC>\n          { BEGIN(INITIAL); }
<PREPROC>.           { /* ignore preprocessor lines */ }

{option}             { BEGIN(OPTION); }
<OPTION>{linewrap}   { /* continue to next line */ }
<OPTION>{optstr}     { return MYLEX_OPTION; }
<OPTION>\n           { BEGIN(INITIAL); }
<OPTION>.            { }

{bom}                { /* ignore byte order mark */ }
[[:space:]]          { }

{identifier}         { tokens_read = 1; return MYLEX_TOKEN; }
"..."                { tokens_read = 1; return MYLEX_TOKEN; }
{other}              { tokens_read = 1; return MYLEX_TOKEN; }

";"                  { /* return if statement was not empty */
    if (tokens_read) {
        tokens_read = 0;
        return MYLEX_SEMICOLON;
    }
}

.                    { illegal_char(); return MYLEX_ERROR; }

%%

/* save error message */
static void illegal_char()
{
    const char *fmt = "illegal character [%c] at line %d";
    const size_t buflen = sizeof(errbuf) - 1;
    unsigned char c = (unsigned char)*yytext;

    if (!isprint((int)*yytext)) {
        fmt = "illegal character [0x%02X] at line %d";
    }

#ifdef _WIN32
    _snprintf_s(errbuf, sizeof(errbuf), buflen, fmt, c, yylineno);
#else
    snprintf(errbuf, buflen, fmt, c, yylineno);
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
