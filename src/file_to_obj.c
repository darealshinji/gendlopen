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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# pragma comment(lib, "ws2_32")
# include <direct.h>
# include <winsock2.h>
# define strcasecmp  _stricmp
# define strncasecmp _strnicmp
#else
# include <arpa/inet.h>
# include <endian.h>
# include <strings.h>
# include <unistd.h>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
# define BSWAP32(x)  _byteswap_ulong(x)
#else
# define BSWAP32(x)  __builtin_bswap32(x)
#endif

#ifdef _MSC_VER
# if defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86)
#  define htole32(x)  x
# else
#  define htole32(x)  BSWAP32(htonl(x))
# endif
#endif

#ifdef _WIN32
# define IS_SEP(c)   (c == '\\' || c == '/')
#else
# define IS_SEP(c)   (c == '/')
#endif

/* case-insensitive string checks */
#define STREQ(a,b)       (strcasecmp(a,b) == 0)
#define STRBEG(STR,PFX)  (strncasecmp(STR, PFX, sizeof(PFX)-1) == 0)

/* write entire buffer to stream */
#define WRITE_BUF(BUFFER, STREAM) \
    write_data(&BUFFER, sizeof(BUFFER), STREAM)

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
const uint8_t mx64[2] = { 0x64, 0x86 };

/* IMAGE_FILE_MACHINE_I386 = 0x014C */
const uint8_t mx86[2] = { 0x4C, 0x01 };

/* IMAGE_FILE_MACHINE_ARM64 = 0xAA64 */
const uint8_t marm64[2] = { 0x64, 0xAA };

/* IMAGE_FILE_MACHINE_ARMNT = 0x01C4 */
const uint8_t marmnt[2] = { 0xC4, 0x01 };

/* IMAGE_FILE_MACHINE_UNKNOWN */
const uint8_t munknown[2] = { 0x00, 0x00 };


/* concatenate str1 + str2 and return copy */
char *gluestr(const char *str1, const char *str2)
{
    char *buf = malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(buf, str1);
    strcat(buf, str2);
    return buf;
}

FILE *open_file(const char *name, const char *mode)
{
    FILE *fp = fopen(name, mode);

    if (!fp) {
        perror("fopen()");
        fprintf(stderr, "(%s)\n", name);
        exit(1);
    }

    return fp;
}

void write_data(const void *ptr, const size_t size, FILE *fp)
{
    if (fwrite(ptr, 1, size, fp) != size) {
        perror("fwrite()");
        exit(1);
    }
}

void set_le_uint32(uint8_t *buf, const size_t offset, const uint32_t val)
{
    uint32_t le32val = htole32(val);
    memcpy(buf + offset, &le32val, 4);
}

const char *simple_basename(const char *str)
{
    /* get basename */
    for (const char *p = str; *p != 0; p++) {
        if (IS_SEP(*p) && *(p+1) != 0) {
            str = p + 1;
        }
    }

    return str;
}

char *symbolic_name(const char *name)
{
    char *p, *out;

    name = simple_basename(name);
    out = malloc(strlen(name) + 1);

    for (p = out; *name != 0; name++, p++) {
        if (isalnum(*name)) {
            *p = (char)tolower(*name);
        } else {
            *p = '_';
        }
    }

    *p = 0;

    printf("symbol: %s\n", out);

    return out;
}

/**
 * https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * Write data to COFF object file:
 *
 * COFF header (20 bytes)
 * section table (40 bytes)
 *
 * file data size (4 bytes Big Endian)
 * file data (n bytes)
 * NUL byte to terminate text data
 *
 * symbol table (18 bytes)
 *
 * string table size (4 bytes)
 * string table (symbol name string length + NUL byte)
 */
void save_to_coff(const char *infile, const char *outfile, const uint8_t *machine, const char *symbol)
{
    FILE *fpIn, *fpOut;
    char *outTmp = NULL;
    char *symTmp = NULL;
    long raw_data_size, hSizeOfRawData;
    uint8_t mangle;
    uint32_t leSizeOfStringTable, hPointerToSymbolTable;
    uint32_t size_of_symbol, be_raw_data_size;
    size_t nread;

    uint8_t buffer[512] = {0};

    uint8_t coff_header[20] = {
        0, 0,           /* Machine */
        0x01, 0,        /* NumberOfSections (1) */
        0, 0, 0, 0,     /* TimeDateStamp */
        0, 0, 0, 0,     /* PointerToSymbolTable */
        0x01, 0, 0, 0,  /* NumberOfSymbols (1) */
        0, 0,           /* SizeOfOptionalHeader */
        0x01, 0x02      /* Characteristics
                          (IMAGE_FILE_RELOCS_STRIPPED |
                           IMAGE_FILE_DEBUG_STRIPPED) */
    };

    const uint8_t off_PointerToSymbolTable = 8;

    uint8_t section_table[40] = {
        '.','r','d','a','t','a', 0, 0, /* Name (.rdata) */
        0, 0, 0, 0,       /* VirtualSize */
        0, 0, 0, 0,       /* VirtualAddress */
        0, 0, 0, 0,       /* SizeOfRawData */
        0x3C, 0, 0, 0,    /* PointerToRawData (offset 60) */
        0, 0, 0, 0,       /* PointerToRelocations */
        0, 0, 0, 0,       /* PointerToLinenumbers */
        0, 0,             /* NumberOfRelocations */
        0, 0,             /* NumberOfLinenumbers */
        0x40, 0, 0, 0x40  /* Characteristics
                            (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_READ) */
    };

    const uint8_t off_SizeOfRawData = 16;

    uint8_t symbol_table[18] = {
        0, 0, 0, 0,     /* Name (assume it's longer than 8 bytes) */
        0x04, 0, 0, 0,  /* Offset into string table */
        0, 0, 0, 0,     /* Value */
        0x01, 0,        /* SectionNumber */
        0, 0,           /* Type */
        0x02,           /* StorageClass (external) */
        0               /* NumberOfAuxSymbols */
    };

    /* open files */
    fpIn = open_file(infile, "rb");

    if (outfile) {
        fpOut = open_file(outfile, "wb");
    } else {
        outTmp = gluestr(infile, ".obj");
        fpOut = open_file(outTmp, "wb");
        free(outTmp);
    }

    /* get file size */
    fseek(fpIn, 0, SEEK_END);

    if ((raw_data_size = ftell(fpIn)) == -1) {
        perror("ftell()");
        exit(1);
    }

    rewind(fpIn);

    /* file size + extra NUL byte + 4 bytes for data size */
    hSizeOfRawData = raw_data_size + 5;

    /* COFF header */
    memcpy(&coff_header, machine, 2);
    hPointerToSymbolTable = sizeof(coff_header) + sizeof(section_table) + hSizeOfRawData;
    set_le_uint32((uint8_t *)&coff_header, off_PointerToSymbolTable, hPointerToSymbolTable);
    WRITE_BUF(coff_header, fpOut);

    /* section table */
    set_le_uint32((uint8_t *)&section_table, off_SizeOfRawData, hSizeOfRawData);
    WRITE_BUF(section_table, fpOut);

    /* data size */
    be_raw_data_size = htonl(raw_data_size);
    write_data(&be_raw_data_size, 4, fpOut);

    /* copy file data */
    while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
        write_data(&buffer, nread, fpOut);
    }

    /* terminating NUL byte */
    fputc(0x00, fpOut);

    /* symbol table */
    WRITE_BUF(symbol_table, fpOut);

    /* string table (including 4 bytes table length) */
    if (!symbol) {
        symbol = symTmp = symbolic_name(infile);
    }
    size_of_symbol = (uint32_t)strlen(symbol) + 1; /* counting NUL byte */
    mangle = (machine == mx86) ? 1 : 0; /* on x86 symbols are prefixed with underscores */
    leSizeOfStringTable = htole32(4 + mangle + size_of_symbol);
    WRITE_BUF(leSizeOfStringTable, fpOut);

    if (mangle) {
        fputc('_', fpOut);
    }
    write_data(symbol, size_of_symbol, fpOut);

    fclose(fpOut);
    fclose(fpIn);

    if (symTmp) {
        free(symTmp);
    }
}


#ifdef CUSTOM_MAIN

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

#else

void print_help(const char *exe)
{
#ifdef _WIN32
    exe = simple_basename(exe);
#endif

    printf("usage: %s [--machine=TARGET] FILE [FILE2 [..]]\n"
           "       %s --help\n"
           "\n"
           "TARGET values: ARM ARM64 X64 X86 NONE\n\n", exe, exe);
}

void try_help(const char *msg, const char *exe)
{
#ifdef _WIN32
    exe = simple_basename(exe);
#endif

    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
    fprintf(stderr, "Try `%s --help' for more information.\n", exe);
}

int main(int argc, char **argv)
{
    char *p;
    const uint8_t *machine = MDEF;
    int argind = 0;

    if (argc < 2) {
        try_help("No arguments.", argv[0]);
        return 1;
    }

    /* parse arguments */
    for (int i = 1; i < argc; i++) {
        if (!STRBEG(argv[i], "--")) {
            argind = i;
            break;
        }

        p = argv[i] + 2;

        if (STREQ(p, "help")) {
            print_help(argv[0]);
            return 0;
        }
        else if (STREQ(p, "machine") || STREQ(p, "machine=")) {
            fprintf(stderr, "missing argument: %s\n", argv[i]);
            try_help(NULL, argv[0]);
            return 1;
        }
        else if (STRBEG(p, "machine=")) {
            p += sizeof("machine=") - 1;

            if (STREQ(p, "X86")) {
                machine = mx86;
            } else if (STREQ(p, "X64")) {
                machine = mx64;
            } else if (STREQ(p, "ARM")) {
                machine = marmnt;
            } else if (STREQ(p, "ARM64")) {
                machine = marm64;
            } else if (STREQ(p, "NONE")) {
                machine = munknown;
            } else {
                fprintf(stderr, "unknown argument for --machine: %s\n", p);
                try_help(NULL, argv[0]);
                return 1;
            }
            continue;
        }

        fprintf(stderr, "unknown option: %s\n", argv[i]);
        try_help(NULL, argv[0]);
        return 1;
    }

    if (argind == 0) {
        try_help("missing input file(s)", argv[0]);
        return 1;
    }

    while (argind < argc) {
        save_to_coff(argv[argind++], NULL, machine, NULL);
    }

    return 0;
}

#endif
