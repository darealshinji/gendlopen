/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2026 Carsten Janssen

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

#ifndef _OPT_PARSER_H_
#define _OPT_PARSER_H_

#ifndef __cplusplus
# include <stdbool.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <strings.h>
#endif


/**
parse arguments:
 - options begin with "-" or "--" or "/"
 - single-character options begin with "-" or "/"
 - "-", "--" and "/" are valid non-option arguments
 - single-character options are without argument separator (-Xfoo)
 - multi-character options are with argument separator "=" or ":" (-foo=bar)
 - arguments can be next entry in argv list (-foo bar, -X foo)

I recommend using uppercase letters for single-character options and
lowercase letters for multi-character options.
Grouping single-character flags ("-a -b -c" to "-abc") like in POSIX getopt()
is not possible since multi-character options may also be prefixed with a single dash.
**/


/* single dash prefix */
//#define OPT_ENABLE_SHORTOPT

/* double-dash prefixes on multi-character options */
//#define OPT_ENABLE_LONGOPT

/* Windows style options (/foo:bar) */
//#define OPT_ENABLE_WIN32OPT

/* ignore case when parsing options */
//#define OPT_CASE_INSENSITIVE

/* all types of options */
//#define OPT_ENABLE_ALLOPTS


/* define `OPT_ENABLE_ASSERT' to build with assertions */
#ifdef OPT_ENABLE_ASSERT
# include <assert.h>
# define OPT_ASSERT(x)  assert(x)
#else
# define OPT_ASSERT(x)  /**/
#endif

#define OPT_INLINE  static inline


struct opt_args {
    /* set by user */
    int          argc;     /* argument count */
    char       **argv;     /* argument vector */
    void (*errhandle)(const char *msg);  /* pointer to custom error handle function */

    /* set by functions */
    const char  *current;  /* pointer to current argv[] entry or NULL if opt_iterate() has failed */
    const char  *value;    /* pointer to option's argument value or NULL if opt_get_arg() has failed */
    int          iter;     /* current argv[] index */
    uint8_t      pfxlen;   /* current option's prefix length (0-2) */
};


/**
 * initialize struct data
 * a: pointer to struct
 * argc: number of argument vector entries
 * argv: argument vector
 * skip: number of arguments to skip, NOT counting argv[0]
 * errhandle: pointer to custom error handle function (optional)
 */
OPT_INLINE void opt_init(struct opt_args *a, int argc, char **argv, int skip, void (*errhandle)(const char *msg));


/**
 * iterate arguments
 * a: pointer to struct
 *
 * Sets "a->current" to the next argument in line and returns "true".
 * If no more arguments are left, "false" is returned and "a->current" is set to NULL.
 */
OPT_INLINE bool opt_iterate(struct opt_args *a);


/**
 * check if current argument equals "opt"
 * a: pointer to struct
 * opt: option string to check (without option prefixes)
 *
 * Returns "true" on match, otherwise "false".
 */
OPT_INLINE bool opt_arg_eq(struct opt_args *a, const char *opt);


/**
 * check if current argument equals "opt" and save pointer to the argument value
 * a: pointer to struct
 * opt: option string to check (without option prefixes)
 * len: length of "opt" in bytes, excluding terminating NUL byte
 *
 * Returns "true" on match and saves pointer to the argument's value at "a->value".
 * Otherwise "false" is returned and "a->value" is set to NULL.
 */
OPT_INLINE bool opt_get_arg(struct opt_args *a, const char *opt, size_t len);



/* enable all option types */
#ifdef OPT_ENABLE_ALLOPTS
# undef OPT_ENABLE_SHORTOPT
# undef OPT_ENABLE_LONGOPT
# undef OPT_ENABLE_WIN32OPT
# define OPT_ENABLE_SHORTOPT
# define OPT_ENABLE_LONGOPT
# define OPT_ENABLE_WIN32OPT
#endif

/* default to short options */
#if !defined(OPT_ENABLE_SHORTOPT) && \
    !defined(OPT_ENABLE_LONGOPT) && \
    !defined(OPT_ENABLE_WIN32OPT)
# define OPT_ENABLE_SHORTOPT
#endif

/* set string comparison functions */
#ifdef OPT_CASE_INSENSITIVE
# ifdef _WIN32
#  define OPT_STRCMP   _stricmp
#  define OPT_STRNCMP  _strnicmp
# else
#  define OPT_STRCMP   strcasecmp
#  define OPT_STRNCMP  strncasecmp
# endif
#else
# define OPT_STRCMP    strcmp
# define OPT_STRNCMP   strncmp
#endif


OPT_INLINE
void opt_init(struct opt_args *a, int argc, char **argv, int skip, void (*errhandle)(const char *))
{
    OPT_ASSERT(a != NULL);
    OPT_ASSERT(argc > 0);
    OPT_ASSERT(argv != NULL);

    a->argc      = argc;
    a->argv      = argv;
    a->errhandle = errhandle;
    a->current   = NULL;
    a->value     = NULL;
    a->iter      = (skip < 1) ? 0 : skip;
    a->pfxlen    = 0;
}


OPT_INLINE
bool opt_iterate(struct opt_args *a)
{
    OPT_ASSERT(a != NULL);

    /* increment */
    a->iter++;

    if (a->iter >= a->argc) {
        return false;
    }

    a->current = a->argv[a->iter];
    a->value = NULL;

    const char *p = a->current;

#ifdef OPT_ENABLE_SHORTOPT
    /* short option */
    if (p[0] == '-' &&
        p[1] != '-' &&
        p[1] != 0)
    {
        a->pfxlen = 1;
        return true;
    }
#endif

#ifdef OPT_ENABLE_LONGOPT
    /* long option */
    if (p[0] == '-' &&
        p[1] == '-' &&
        p[2] != 0)
    {
        a->pfxlen = 2;
        return true;
    }
#endif

#ifdef OPT_ENABLE_WIN32OPT
    /* forward slash option */
    if (p[0] == '/' &&
        p[1] != 0)
    {
        a->pfxlen = 1;
        return true;
    }
#endif

    /* no option prefix */
    a->pfxlen = 0;

    return true;
}


OPT_INLINE
bool opt_arg_eq(struct opt_args *a, const char *opt)
{
    OPT_ASSERT(a != NULL);
    OPT_ASSERT(a->current != NULL);
    OPT_ASSERT(a->pfxlen <= 2);
    OPT_ASSERT(opt != NULL);

    if (opt[0] == 0 || a->pfxlen == 0) {
        return false;
    }

    return (OPT_STRCMP(a->current + a->pfxlen, opt) == 0);
}


OPT_INLINE
void _opt_handle_error(struct opt_args *a, const char *fmt, const char *pfx, const char *opt)
{
    char buf[128];

    if (a->errhandle) {
#ifdef _WIN32
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, pfx, opt);
#else
        snprintf(buf, sizeof(buf), fmt, pfx, opt);
#endif
        a->errhandle(buf);
    } else {
        fprintf(stderr, "error: ");
        fprintf(stderr, fmt, pfx, opt);
        fprintf(stderr, "\n");
        exit(1);
    }
}


OPT_INLINE
bool opt_get_arg(struct opt_args *a, const char *opt, size_t len)
{
    OPT_ASSERT(a != NULL);
    OPT_ASSERT(a->current != NULL);
    OPT_ASSERT(opt != NULL);
    OPT_ASSERT(a->pfxlen <= 2);

    char pfx[3] = { 0, 0, 0 };

    if (len == 0 || a->pfxlen == 0) {
        return false;
    }

#ifdef OPT_ENABLE_LONGOPT
    /* only short prefix for single-character options */
    if (opt[1] == 0 && a->pfxlen != 1) {
        return false;
    }
#endif

    /* save prefix characters and skip prefix */
    const char *p = a->current;
    pfx[0] = p[0];
    p++;

#ifdef OPT_ENABLE_LONGOPT
    if (a->pfxlen == 2) {
        pfx[1] = p[1];
        p++;
    }
#endif

    if (OPT_STRNCMP(p, opt, len) != 0) {
        /* not the right option string */
        return false;
    } else if (p[len] == 0) {
        /* get next argv[] entry */
        a->iter++;

        if (a->iter >= a->argc) {
            _opt_handle_error(a, "option '%s%s' requires an argument", pfx, opt);
            return false;
        }

        a->value = a->argv[a->iter];
    } else if (len == 1) {
        /* -Xabc */
        a->value = p + len;
    } else if (p[len] == '=') {
        /* -foo=abc */
        a->value = p + len + 1;
    }
#ifdef OPT_ENABLE_WIN32OPT
    else if (p[len] == ':') {
        /* /foo:abc */
        a->value = p + len + 1;
    }
#endif
    else {
        return false;
    }

    /* empty argument */
    if (a->value[0] == 0) {
        _opt_handle_error(a, "option '%s%s' has an empty argument", pfx, opt);
        return false;
    }

    return true;
}


#ifdef __cplusplus
//} /* extern "C" */
#endif


// cc -Wall -O2 -DOPT_ENABLE_ASSERT -DOPT_ENABLE_ALLOPTS -DOPT_PARSER_TEST -xc opt_parser.h
#ifdef OPT_PARSER_TEST

static void print_error(const char *msg) {
    fprintf(stderr, "error: %s: %s\n", __FUNCTION__, msg);
    exit(1);
}

int main(int argc, char **argv)
{
    struct opt_args a;

    /* initialize struct */
    opt_init(&a, argc, argv, 0, print_error);

    /* start loop */
    while (opt_iterate(&a))
    {
        /* multi-character option with argument */
        if (opt_get_arg(&a, "foo", 3)) {
            printf("foo == %s\n", a.value);
        }
        /* single-character option with argument */
        else if (opt_get_arg(&a, "X", 1)) {
            printf("X == %s\n", a.value);
        }
        /* single-character flag */
        else if (opt_arg_eq(&a, "Y")) {
            puts("flag 'Y' set");
        }
        /* multi-character flag */
        else if (opt_arg_eq(&a, "bar")) {
            puts("flag 'bar' set");
        }
        /* prefix length of 0 means it's a non-option argument */
        else if (a.pfxlen == 0) {
            printf("non-option argument: %s\n", a.current);
        }
        /* unknown option */
        else {
            fprintf(stderr, "unknown option: %s\n", a.current);
        }
    }

    return 0;
}

#endif /* OPT_PARSER_TEST */

#endif /* _OPT_PARSER_H_ */

