%top{
#include <ctype.h>
#include "lex.h"
}

%{
/* forward declarations */
static int handle_ast_begin();
void set_illegal_char();
%}

/* definitions */

bom                 "\xEF\xBB\xBF"
col                 [\x1B][\[][0-9;]*[m]
split               "\\"[[:space:]]*?"\n"
ast_begin           ^({bom})?({col})?"TranslationUnitDecl"({col})?({col})?" 0x".*
funcvar             ^({col})?[|`]"-"({col})?({col})?("Function"|"Var")"Decl"({col})?({col})?" 0x".*
parmvar             ^({col})?[| ]" "[|`]"-"({col})?({col})?"ParmVarDecl"({col})?({col})?" 0x".*
option              ^({bom})?"%option"[[:space:]]
optstr              [^[:space:]]+
preproc             ^({bom})?[[:space:]]*?"#"
identifier          [a-zA-Z0-9_]+

%option noinput nounput noyywrap yylineno nounistd never-interactive

%x AST
%x OPTION
%x COMMENT
%x C_COMMENT
%x PRE
%x PRE_COMMENT

/* rules */
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

"extern"                    { /* ignore "extern" keyword */ }
"struct"                    { return LEX_STRUCT; }
"union"                     { return LEX_UNION; }
"enum"                      { return LEX_ENUM; }
"typedef"                   { return LEX_TYPEDEF; }

{identifier}                { return LEX_ID; }
[*,()\[\]]                  { return LEX_OTHER; }
"..."                       { return LEX_OTHER; }

"{"                         { return '{'; }
"}"                         { return '}'; }
"="                         { return '='; }
";"                         { return ';'; }

.                           { set_illegal_char(); return LEX_ERROR; }

%%

/* user code */

#ifdef _MSC_VER
# define SNPRINTF(STR, LEN, FMT, ...)  _snprintf_s(STR, LEN, _TRUNCATE, FMT, __VA_ARGS__)
#else
# define SNPRINTF(STR, LEN, FMT, ...)  snprintf(STR, LEN, FMT, __VA_ARGS__)
#endif

static char errbuf[128] = {0};

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

/* return error message */
const char *lex_errmsg() {
    return *errbuf ? errbuf : NULL;
}

/* save error message */
void set_illegal_char()
{
    unsigned char c = (unsigned char)*yytext;
    const char *fmt = "illegal character [%c] at line %d";

    if (!isgraph((int)c)) {
        fmt = "illegal character [0x%02X] at line %d";
    }

    SNPRINTF(errbuf, sizeof(errbuf), fmt, c, yylineno);
}
