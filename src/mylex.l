%{
#include <ctype.h>

static int token_count = 0;
static char errbuf[128] = {0};
static void illegal_char();

enum {
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_OPTION,
    MYLEX_TOKEN,
    MYLEX_SEMICOLON,
    MYLEX_ERROR
};
%}

bom                  "\xEF\xBB\xBF"
col                  [\x1B][\[][0-9;]*[m]

ast_begin            ^({bom})?({col})?"TranslationUnitDecl"({col})?({col})?" 0x".*
funcvar              ^({col})?[|`]"-"({col})?({col})?("Function"|"Var")"Decl".*
parmvar              ^({col})?[| ]" "[|`]"-"({col})?({col})?"ParmVarDecl".*

option               ^({bom})?"%option"[ \t].*
identifier           [a-zA-Z0-9_]+
other                [*,()\[\]]

%option noinput nounput noyywrap yylineno nounistd never-interactive

%x C_COMMENT
%x CLANG_AST

%%

{ast_begin}          { /* clang AST identifier in first line */
    if (yylineno == 1) {
        BEGIN(CLANG_AST);
        return MYLEX_AST_BEGIN;
    }
}

<CLANG_AST>{funcvar} { return MYLEX_OK; }
<CLANG_AST>{parmvar} { return MYLEX_OK; }
<CLANG_AST>\n        { }
<CLANG_AST>.         { }

"/*"                 { BEGIN(C_COMMENT); }
<C_COMMENT>"*/"      { BEGIN(INITIAL); }
<C_COMMENT>\n        { }
<C_COMMENT>.         { }

{option}             { token_count = 0; return MYLEX_OPTION; }

"//".*               { /* single line comment */ }
{bom}                { /* ignore byte order mark */ }
[[:space:]]          { }

{identifier}         { token_count++; return MYLEX_TOKEN; }
"..."                { token_count++; return MYLEX_TOKEN; }
{other}              { token_count++; return MYLEX_TOKEN; }

";"                  { /* return if statement was not empty */
    if (token_count > 0) {
        token_count = 0;
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
