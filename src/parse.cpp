/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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

#include <iterator>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "parse.hpp"


/* placeholder characters */
#define TYPE   "\x01"
#define SYMBOL "\x02"


typedef struct _seq {
    const char *front;      /* first part */
    const char *middle;     /* the part where the iterator is */
    const char *end;        /* last element */
    const size_t min_size;  /* minimum vector size (pattern sequence length in bytes) */
    const int dist_front;   /* distance between front and iterator */
    const int dist_end;     /* distance between end and iterator */
} seq_t;


namespace /* anonymous */
{
    /* compare vector elements to pattern sequence
     *
     * first element was already checked in `check_prototype()'
     * and is an identifier and iterator is NOT v.begin() */
    bool check_pattern(vstring_t &v, iter_t it, const seq_t &sq)
    {
        /* compare vector element against sequence element */
        auto matching = [] (const char &e_vec, const char &e_seq) -> bool {
            return (e_vec == e_seq ||
                /* vector element must be identificator */
                (e_seq == *SYMBOL && parse::is_ident(e_vec)));
        };

        /* minimum vector size */
        if (v.size() < sq.min_size) {
            return false;
        }

        /* boundaries */
        if (it != v.end() &&  /* vector contains parentheses/brackets/etc. */
            (std::distance(v.begin(), it) < sq.dist_front ||
             std::distance(it, v.end()) < sq.dist_end))
        {
            return false;
        }

        /* front */
        if (sq.front[1] == *SYMBOL && /* front part must hold identificator */
            !parse::is_ident(utils::str_front(*(it-1))))
        {
            return false;
        }

        /* end */
        switch (sq.end[1])
        {
        case 0: /* 1 element */
            if (matching(utils::str_front(v.back()), sq.end[0])) {
                break;
            }
            return false;

        default:
            /* 2 elements */
            if (matching(utils::str_front(*(v.end()-1)), sq.end[1]) &&
                matching(utils::str_front(*(v.end()-2)), sq.end[0]))
            {
                break;
            }
            return false;
        }

        /* middle */
        for (auto p = sq.middle; *p != 0; p++, it++) {
            if (!matching(utils::str_front(*it), *p)) {
                return false;
            }
        }

        return true;
    }

} /* end anonymous namespace */


vstring_t::iterator parse::find_first_not_pointer_or_ident(vstring_t &v)
{
    for (auto it = v.begin(); it != v.end(); it++) {
        char c = utils::str_front(*it);

        if (c != '*' && !is_ident(c)) {
            return it;
        }
    }

    return v.end();
}


#define XSTRLEN(x) (sizeof(x)-1)

#define CHECK_FRONT(FRONT) \
    ((XSTRLEN(FRONT) == 1 && *FRONT == *TYPE) || \
     (XSTRLEN(FRONT) == 2 && *FRONT == *TYPE && FRONT[1] == *SYMBOL))

#define MKFUNC(NAME,FRONT,MIDDLE,END) \
    bool NAME(vstring_t &v, const iter_t &it) \
    { \
        static_assert(CHECK_FRONT(FRONT), \
            "macro parameter 'FRONT' must be either 'TYPE' or 'TYPE SYMBOL'"); \
        \
        static_assert(XSTRLEN(END) == 1 || XSTRLEN(END) == 2, \
            "macro parameter 'END' must contain exactly 1 or 2 elements"); \
        \
        const seq_t sq = { \
            .front      = FRONT, \
            .middle     = MIDDLE, \
            .end        = END, \
            .min_size   = XSTRLEN(FRONT) + XSTRLEN(MIDDLE) + XSTRLEN(END), \
            .dist_front = XSTRLEN(FRONT), \
            .dist_end   = XSTRLEN(MIDDLE) + XSTRLEN(END) \
        }; \
        \
        return check_pattern(v, it, sq); \
    }

MKFUNC( parse::is_function,                  TYPE         SYMBOL,     "(", ")"  )
MKFUNC( parse::is_function_parentheses,      TYPE,    "(" SYMBOL ")"  "(", ")"  )
MKFUNC( parse::is_function_paren_pointer1,   TYPE,   "(*" SYMBOL      "(", "))" )
MKFUNC( parse::is_function_paren_pointer2,   TYPE,  "(**" SYMBOL      "(", "))" )

MKFUNC( parse::is_function_pointer,          TYPE,   "(*" SYMBOL ")"  "(", ")"  )
MKFUNC( parse::is_function_pointer_no_name,  TYPE,   "(*" /****/ ")"  "(", ")"  )

MKFUNC( parse::is_object,                    TYPE,    "", SYMBOL                )
MKFUNC( parse::is_array,                     TYPE         SYMBOL,     "[", "]"  )
MKFUNC( parse::is_array_no_name,             TYPE,        /****/      "[", "]"  )

