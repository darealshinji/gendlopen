#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum {
    //MYLEX_EOF = 0,
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_AST_PARMVAR,
    MYLEX_OPTION,
    MYLEX_ID,
    MYLEX_OTHER,
    MYLEX_SEMICOLON,
    MYLEX_ERROR
};

extern char *yytext;

int mylex(FILE *fp);
const char *mylex_lasterror();

#ifdef __cplusplus
}
#endif
