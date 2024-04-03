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

#ifndef _MISC_HPP_
#define _MISC_HPP_

#include <string>
#include <vector>


/* ANSI color codes used in the Clang AST output */

/* escaped variants for std::regex */
#define COL(x)    "\x1B\\[" #x "m"
#define C0        COL(0)          /* default */
#define CGREEN    COL(0;32)       /* green */
#define CFGREEN   COL(0;1;32)     /* fat green */
#define CFBLUE    COL(0;1;36)     /* fat blue */

/* unescaped variants for std::string */
#define sCOL(x)   "\x1B[" #x "m"
#define sC0       sCOL(0)         /* default */
#define sCORANGE  sCOL(0;33)      /* orange */
#define sCFGREEN  sCOL(0;1;32)    /* fat green */


/* assume */
#if defined(__has_cpp_attribute) && __has_cpp_attribute(assume) >= 202207L
    #define ASSUME(x)    [[assume(x)]]
#elif defined(__clang__)
    #define ASSUME(x)    __builtin_assume(x)
#elif defined(_MSC_VER)
    #define ASSUME(x)    __assume(x)
#elif defined(__GNUC__) && __GNUC__ >= 13
    #define ASSUME(x)    [[gnu::assume(x)]]
#else
    #define ASSUME(x)    /**/
#endif


/* typedefs */

typedef struct {
    std::string type;
    std::string symbol;
    std::string args;
    std::string notype_args;
} proto_t;

using vproto_t = std::vector<proto_t>;
using vstring_t = std::vector<std::string>;


/* enum for outout format */

namespace output
{
    typedef enum {
        c,
        cxx,
        minimal,
        minimal_cxx
    } format;
}

#endif // _MISC_HPP_
