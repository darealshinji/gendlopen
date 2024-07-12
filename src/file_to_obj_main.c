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

#include <stddef.h>
#include <stdint.h>

void save_to_coff(const char *infile, const char *outfile, const uint8_t *machine, const char *symbol);

/* set default machine type */
#if defined(_M_X64) || defined(_M_AMD64)
# define MDEF mx64
#elif defined(_M_IX86)
# define MDEF mx86
#elif defined(_M_ARM64)
# define MDEF marm64
#elif defined(_M_ARM) && defined(_M_THUMB)
# define MDEF marmnt
#else
# define MDEF munknown
#endif

/* IMAGE_FILE_MACHINE_AMD64 = 0x8664 */
static const uint8_t mx64[2] = { 0x64, 0x86 };

/* IMAGE_FILE_MACHINE_I386 = 0x014C */
static const uint8_t mx86[2] = { 0x4C, 0x01 };

/* IMAGE_FILE_MACHINE_ARM64 = 0xAA64 */
static const uint8_t marm64[2] = { 0x64, 0xAA };

/* IMAGE_FILE_MACHINE_ARMNT = 0x01C4 */
static const uint8_t marmnt[2] = { 0xC4, 0x01 };

/* IMAGE_FILE_MACHINE_UNKNOWN */
static const uint8_t munknown[2] = { 0x00, 0x00 };


int main()
{
    const uint8_t *machine = MDEF;

#define SAVE_FILE(FILE,SYM) \
    save_to_coff("../src/templates/" FILE, "templ_" FILE ".obj", machine, SYM "_raw");

    SAVE_FILE("filename_macros.h", "filename_macros");
    SAVE_FILE("license.h",         "license");
    SAVE_FILE("common.h",          "common_header");
    SAVE_FILE("c.h",               "c_header");
    SAVE_FILE("c.c",               "c_body");
    SAVE_FILE("cxx.hpp",           "cxx_header");
    SAVE_FILE("cxx.cpp",           "cxx_body");
    SAVE_FILE("minimal.h",         "min_c_header");
    SAVE_FILE("minimal_cxxeh.hpp", "min_cxx_header");

    return 0;
}
