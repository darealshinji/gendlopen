#ifndef _CLANG_AST_H_
#define _CLANG_AST_H_

/* ANSI color codes used in the Clang AST output */

/* escaped variants for regex */
#define COL(x)    "\x1B\\[" #x "m"
#define C0        COL(0)        /* default */
#define CGREEN    COL(0;32)     /* green */
#define CFGREEN   COL(0;1;32)   /* fat green */
#define CFBLUE    COL(0;1;36)   /* fat blue */

/* unescaped variants for std::string */
#define sCOL(x)   "\x1B[" #x "m"
#define sC0       sCOL(0)       /* default */
#define sCORANGE  sCOL(0;33)    /* orange */
#define sCFGREEN  sCOL(0;1;32)  /* fat green */

#endif //_CLANG_AST_H_
