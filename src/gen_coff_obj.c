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

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS

#ifdef _MSC_VER
# include <direct.h>
#else
# include <endian.h>  /* for testing on Linux */
# include <unistd.h>
#endif
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# if BYTE_ORDER == BIG_ENDIAN
#  define htole32(x) _byteswap_ulong(x)
# else
#  define htole32(x) x
# endif
# if defined(_M_X64) || defined(_M_AMD64)
#  define COFF_MACHINE  0x64, 0x86,  /* IMAGE_FILE_MACHINE_AMD64 = 0x8664 */
# elif defined(_M_IX86)
#  define COFF_MACHINE  0x4C, 0x01,  /* IMAGE_FILE_MACHINE_I386 = 0x014C */
# elif defined(_M_ARM64)
#  define COFF_MACHINE  0x64, 0xAA,  /* IMAGE_FILE_MACHINE_ARM64 = 0xAA64 */
# elif defined(_M_ARM) && defined(_M_THUMB)
#  define COFF_MACHINE  0xC4, 0x01,  /* IMAGE_FILE_MACHINE_ARMNT = 0x01C4 */
# elif defined(_M_ARM)
#  define COFF_MACHINE  0xC0, 0x01,  /* IMAGE_FILE_MACHINE_ARM = 0x01C0 */
# endif
#endif
#ifndef COFF_MACHINE
# define COFF_MACHINE  0, 0,  /* IMAGE_FILE_MACHINE_UNKNOWN = 0x0000 (any machine) */
#endif

#define OFFSET_POINTERTOSYMBOLTABLE  8
#define OFFSET_SIZEOFRAWDATA  16


/**
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * Write data to COFF object file:
 *
 * COFF header (20 bytes)
 * section table (40 bytes)
 *
 * file data (n bytes)
 * NUL byte for const char[] data (1 byte)
 *
 * symbol table (18 bytes)
 *
 * string table size (4 bytes)
 * string table (symbol name string length + NUL byte)
 */
void save_to_coff(const char *fin, const char *din, const char *fout, const char *dout, const char *symbol)
{
    FILE *fpIn, *fpOut;

    long hSizeOfRawData;
    uint32_t leSizeOfRawData, leSizeOfStringTable;
    uint32_t hPointerToSymbolTable, lePointerToSymbolTable;
    uint32_t size_of_symbol;
    uint8_t *ptr;
    size_t nread;

    uint8_t buffer[512] = {0};
    const uint8_t nullbyte = 0;

    uint8_t coff_header[20] = {
        COFF_MACHINE    /* Machine */
        0x01, 0,        /* NumberOfSections */
        0, 0, 0, 0,     /* TimeDateStamp */
        0, 0, 0, 0,     /* PointerToSymbolTable */
        0x01, 0, 0, 0,  /* NumberOfSymbols */
        0, 0,           /* SizeOfOptionalHeader */
        0x05, 0         /* Characteristics (relocations stripped, line numbers stripped) */
    };

    uint8_t section_table[40] = {
        '.','d','a','t','a', 0, 0, 0, /* Name */
        0, 0, 0, 0,     /* VirtualSize */
        0, 0, 0, 0,     /* VirtualAddress */
        0, 0, 0, 0,     /* SizeOfRawData */
        0x3C, 0, 0, 0,  /* PointerToRawData */
        0, 0, 0, 0,     /* PointerToRelocations */
        0, 0, 0, 0,     /* PointerToLinenumbers */
        0, 0,           /* NumberOfRelocations */
        0, 0,           /* NumberOfLinenumbers */
        0, 0, 0, 0      /* Characteristics */
    };

    const uint8_t symbol_table[18] = {
        0, 0, 0, 0,     /* Name is longer than 8 bytes */
        0x04, 0, 0, 0,  /* Offset into string table */
        0, 0, 0, 0,     /* Value */
        0x01, 0,        /* SectionNumber */
        0, 0,           /* Type */
        0x02,           /* StorageClass (external) */
        0               /* NumberOfAuxSymbols */
    };

    /* assert on empty strings */
    assert(fin != NULL && *fin != 0);
    assert(fout != NULL && *fout != 0);
    assert(symbol != NULL && *symbol != 0);

    /* open input file */

    if (din && chdir(din) == -1) {
        perror("chdir()");
        fprintf(stderr, "(%s)\n", din);
        exit(1);
    }

    if ((fpIn = fopen(fin, "rb")) == NULL) {
        perror("fopen()");
        exit(1);
    }

    /* get file size */
    fseek(fpIn, 0, SEEK_END);

    if ((hSizeOfRawData = ftell(fpIn)) == -1) {
        perror("ftell()");
        exit(1);
    }

    rewind(fpIn);

    hSizeOfRawData++; /* plus 1 for NUL byte */
    leSizeOfRawData = htole32(hSizeOfRawData);
    ptr = section_table;
    memcpy(ptr + OFFSET_SIZEOFRAWDATA, &leSizeOfRawData, 4);

    /* symbol table offset */
    hPointerToSymbolTable = sizeof(coff_header) + sizeof(section_table) + hSizeOfRawData;
    lePointerToSymbolTable = htole32(hPointerToSymbolTable);
    ptr = coff_header;
    memcpy(ptr + OFFSET_POINTERTOSYMBOLTABLE, &lePointerToSymbolTable, 4);

    /* string table length */
    size_of_symbol = (uint32_t)strlen(symbol) + 1; /* strlen + NUL byte */
    leSizeOfStringTable = htole32(size_of_symbol + 4);

    /* open output file */

    if (dout && chdir(dout) == -1) {
        perror("chdir()");
        fprintf(stderr, "(%s)\n", dout);
        exit(1);
    }

    if ((fpOut = fopen(fout, "wb")) == NULL) {
        perror("fopen()");
        exit(1);
    }

#define WRITE_FPOUT(BUFFER) \
    (fwrite(&BUFFER, 1, sizeof(BUFFER), fpOut) == sizeof(BUFFER))

    /* header */
    if (!WRITE_FPOUT(coff_header) ||
        !WRITE_FPOUT(section_table))
    {
        perror("fwrite()");
        exit(1);
    }

    /* file data */
    while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
        if (fwrite(buffer, 1, nread, fpOut) != nread) {
            perror("fwrite()");
            exit(1);
        }
    }

    /* null byte, symbol table, string table */
    if (!WRITE_FPOUT(nullbyte) ||
        !WRITE_FPOUT(symbol_table) ||
        !WRITE_FPOUT(leSizeOfStringTable) ||
        fwrite(symbol, 1, size_of_symbol, fpOut) != size_of_symbol)
    {
        perror("fwrite()");
        exit(1);
    }

    fclose(fpOut);
    fclose(fpIn);
}

/* takes input files directory as optional argument */
int main(int argc, char **argv)
{
    const char *input_dir = NULL;
    char *cwd = getcwd(NULL, 0);

    if (!cwd) {
        perror("getcwd()");
        exit(1);
    }

    if (argc > 1) {
        input_dir = argv[1];
    }

#define SAVE_TEXTFILE(INPUT, SYMBOL) \
    save_to_coff(INPUT, input_dir, "templ_" INPUT ".obj", cwd, SYMBOL)

    SAVE_TEXTFILE("filename_macros.h", "filename_macros_data");
    SAVE_TEXTFILE("license.h",         "license_data");
    SAVE_TEXTFILE("common.h",          "common_header_data");
    SAVE_TEXTFILE("c.h",               "c_header_data");
    SAVE_TEXTFILE("c.c",               "c_body_data");
    SAVE_TEXTFILE("cxx.hpp",           "cxx_header_data");
    SAVE_TEXTFILE("cxx.cpp",           "cxx_body_data");
    SAVE_TEXTFILE("minimal.h",         "min_c_header_data");
    SAVE_TEXTFILE("minimal_cxxeh.hpp", "min_cxx_header_data");

    free(cwd);

    return 0;
}

