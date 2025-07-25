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
#include "lex.h"

#ifdef _MSC_VER
# define SNPRINTF(STR, LEN, FMT, ...)  _snprintf_s(STR, LEN, _TRUNCATE, FMT, __VA_ARGS__)
#else
# define SNPRINTF(STR, LEN, FMT, ...)  snprintf(STR, LEN, FMT, __VA_ARGS__)
#endif

static char errbuf[128] = {0};
const char *lex_errmsg = errbuf;

static int handle_ast_begin();
void set_illegal_char();

%}

bom                 "\xEF\xBB\xBF"
col                 [\x1B][\[][0-9;]*[m]
split               "\\"[[:space:]]*?"\n"
ast_begin           ^({bom})?({col})?"TranslationUnitDecl"({col})?({col})?" 0x".*
funcvar             ^({col})?[|`]"-"({col})?({col})?("Function"|"Var")"Decl".*
parmvar             ^({col})?[| ]" "[|`]"-"({col})?({col})?"ParmVarDecl".*

option              ^({bom})?"%option"[[:space:]]
optstr              [^[:space:]]+

preproc             ^({bom})?[[:space:]]*?"#"

identifier          [a-zA-Z0-9_]+
other               [*,()\[\]]

%option noinput nounput noyywrap yylineno nounistd never-interactive

%x AST
%x OPTION
%x COMMENT
%x C_COMMENT
%x PRE
%x PRE_COMMENT

%%

{ast_begin}                 { return handle_ast_begin(); }
<AST>{funcvar}              { return LEX_AST_FUNCVAR; }
<AST>{parmvar}              { return LEX_AST_PARMVAR; }
<AST>\n                     { }
<AST>.                      { }

{option}                    { BEGIN(OPTION); /* %option line */ }
<OPTION>{split}             { /* line split */ }
<OPTION>{optstr}            { return LEX_OPTION; }
<OPTION>\n                  { BEGIN(INITIAL); /* end */}
<OPTION>.                   { }

"//"                        { BEGIN(COMMENT); /* simple comment */ }
"/"{split}"/"               { BEGIN(COMMENT); /* simple comment */ }
<COMMENT>{split}            { /* line split */ }
<COMMENT>\n                 { BEGIN(INITIAL); /* end */ }
<COMMENT>.                  { }

"/*"                        { BEGIN(C_COMMENT); /* C comment */ }
"/"{split}"*"               { BEGIN(C_COMMENT); /* C comment */ }
<C_COMMENT>"*/"             { BEGIN(INITIAL); /* end */ }
<C_COMMENT>"*"{split}"/"    { BEGIN(INITIAL); /* end */ }
<C_COMMENT>\n               { }
<C_COMMENT>.                { }

{preproc}                   { BEGIN(PRE); }
<PRE>"/*"                   { BEGIN(PRE_COMMENT); /* C comment within PRE line */ }
<PRE>"/"{split}"*"          { BEGIN(C_COMMENT); /* C comment, end of PRE line */ }
<PRE>"//"                   { BEGIN(COMMENT); /* simple comment */ }
<PRE>"/"{split}"/"          { BEGIN(COMMENT); /* simple comment */ }
<PRE>{split}                { /* line split */ }
<PRE>\n                     { BEGIN(INITIAL); /* end */ }
<PRE>.                      { }

<PRE_COMMENT>"*/"           { BEGIN(PRE); /* continue PRE line */ }
<PRE_COMMENT>"*"{split}"/"  { BEGIN(INITIAL); /* end */ }
<PRE_COMMENT>\n             { BEGIN(C_COMMENT); /* C comment */ }
<PRE_COMMENT>.              { }

{bom}                       { /* UTF-8 byte order mark */ }
[[:space:]]                 { }

"struct"                    { return LEX_STRUCT; }
"union"                     { return LEX_UNION; }
"enum"                      { return LEX_ENUM; }
"typedef"                   { return LEX_TYPEDEF; }

{identifier}                { return LEX_ID; }
{other}                     { return LEX_OTHER; }
"..."                       { return LEX_OTHER; }

"{"                         { return LEX_CURLY_OPEN; }
"}"                         { return LEX_CURLY_CLOSE; }
"="                         { return LEX_EQUAL; }
";"                         { return LEX_SEMICOLON; }

.                           { set_illegal_char(); return LEX_ERROR; }

%%

/* handle begin of Clang AST */
static int handle_ast_begin()
{
    if (yylineno == 1) {
        BEGIN(AST);
        return LEX_AST_BEGIN;
    }

    SNPRINTF(errbuf, sizeof(errbuf),
        "begin of Clang AST found at line %d instead of line 1", yylineno);

    return LEX_ERROR;
}

/* save error message */
void set_illegal_char()
{
    const char *fmt = "illegal character [%c] at line %d";
    unsigned char c = (unsigned char)*yytext;

    if (!isgraph((int)*yytext)) {
        fmt = "illegal character [0x%02X] at line %d";
    }

    SNPRINTF(errbuf, sizeof(errbuf), fmt, c, yylineno);
}
