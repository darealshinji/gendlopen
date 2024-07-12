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

#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86)
# define INCBIN_LITTLE_ENDIAN
#else
# include <winsock2.h>
# pragma comment(lib, "ws2_32")  /* ntohl() */
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
# define INCBIN_EXTERN  extern "C"
#else
# define INCBIN_EXTERN  extern
#endif

/* reference the data */
#define INCBIN(SYMBOL) \
    INCBIN_EXTERN const uint8_t SYMBOL[]; \
    INCBIN_EXTERN const uint32_t SYMBOL##_size_BE; \
    INCBIN_EXTERN const uint32_t SYMBOL##_size_LE

/* receive data size */
#ifdef INCBIN_LITTLE_ENDIAN
# define GETINC_SIZE(SYMBOL)  SYMBOL##_size_LE  /* Little Endian only */
#else
# define GETINC_SIZE(SYMBOL)  ntohl(SYMBOL##_size_BE)  /* any system */
#endif
