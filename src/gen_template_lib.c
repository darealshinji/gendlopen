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
# include <direct.h>
# define strcasecmp  _stricmp
# define strncasecmp _strnicmp
# ifdef __clang__
#  define bswap_32(x) __builtin_bswap32(x)
# else
#  define bswap_32(x) _byteswap_ulong(x)
# endif
#else
# include <endian.h>
# include <strings.h>
# include <unistd.h>
#endif

/* case-insensitive */
#define STREQ(a,b)      (strcasecmp(a,b) == 0)
#define STRBEG(STR,PFX) (strncasecmp(STR, PFX, sizeof(PFX)-1) == 0)

/* write entire buffer to stream */
#define WRITE_BUF(BUFFER, STREAM) \
    write_data(&BUFFER, sizeof(BUFFER), STREAM)

/* use wine on other systems */
#ifdef _WIN32
# define WINE ""
#else
# define WINE "wine "
#endif

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


struct cfg {
    const char *infile;
    const char *indir;
    const char *outfile;
    const char *outdir;
    const char *symbol;
    uint8_t machine[2];
};

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


/* runtime endianness detection */
#ifdef _MSC_VER
inline uint32_t htole32(uint32_t val)
{
    const uint8_t little_endian = 0x01;
    uint16_t word = 0x0001;
    uint8_t *byte = (uint8_t *)&word;

    return (byte[0] == little_endian) ? val : bswap_32(val);
}
#endif

void err_exit(const char *msg1, const char *msg2)
{
    fprintf(stderr, "%s%s\n", msg1, msg2);
    exit(1);
}

void change_dir(const char *dir)
{
    if (dir && *dir && chdir(dir) == -1) {
        perror("chdir()");
        fprintf(stderr, "(%s)\n", dir);
        exit(1);
    }
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

void write_data(const void *ptr, size_t size, FILE *fp)
{
    if (fwrite(ptr, 1, size, fp) != size) {
        perror("fwrite()");
        exit(1);
    }
}

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
void save_to_coff(struct cfg *conf)
{
    FILE *fpIn, *fpOut;

    long hSizeOfRawData;
    uint32_t leSizeOfRawData, leSizeOfStringTable;
    uint32_t hPointerToSymbolTable, lePointerToSymbolTable;
    uint32_t size_of_symbol;
    uint8_t *ptr;
    size_t nread;
    bool mangle;

    uint8_t buffer[512] = {0};
    const uint8_t nullbyte = 0;

    uint8_t coff_header[20] = {
        0, 0,           /* Machine */
        0x01, 0,        /* NumberOfSections */
        0, 0, 0, 0,     /* TimeDateStamp */
        0, 0, 0, 0,     /* PointerToSymbolTable */
        0x01, 0, 0, 0,  /* NumberOfSymbols */
        0, 0,           /* SizeOfOptionalHeader */
        0x05, 0         /* Characteristics (relocations stripped, line numbers stripped) */
    };

    const uint8_t off_PointerToSymbolTable = 8;

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

    const uint8_t off_SizeOfRawData = 16;

    const uint8_t symbol_table[18] = {
        0, 0, 0, 0,     /* Name is longer than 8 bytes */
        0x04, 0, 0, 0,  /* Offset into string table */
        0, 0, 0, 0,     /* Value */
        0x01, 0,        /* SectionNumber */
        0, 0,           /* Type */
        0x02,           /* StorageClass (external) */
        0               /* NumberOfAuxSymbols */
    };

    assert(conf != NULL);

    coff_header[0] = conf->machine[0];
    coff_header[1] = conf->machine[1];

    /* on x86 __cdecl adds underscores to symbol names */
    mangle = (memcmp(conf->machine, mx86, 2) == 0);

    /* open input file */
    change_dir(conf->indir);
    fpIn = open_file(conf->infile, "rb");

    /* get file size */
    fseek(fpIn, 0, SEEK_END);

    if ((hSizeOfRawData = ftell(fpIn)) == -1) {
        perror("ftell()");
        exit(1);
    }

    rewind(fpIn);

    hSizeOfRawData++; /* + NUL byte */
    leSizeOfRawData = htole32(hSizeOfRawData);
    ptr = section_table;
    ptr += off_SizeOfRawData;
    memcpy(ptr, &leSizeOfRawData, 4);

    /* symbol table offset */
    hPointerToSymbolTable = sizeof(coff_header) + sizeof(section_table) + hSizeOfRawData;
    lePointerToSymbolTable = htole32(hPointerToSymbolTable);
    ptr = coff_header;
    ptr += off_PointerToSymbolTable;
    memcpy(ptr, &lePointerToSymbolTable, 4);

    /* string table length */
    size_of_symbol = (uint32_t)strlen(conf->symbol) + 1; /* strlen + NUL byte */

    if (mangle) {
        /* +1 leading underscore */
        size_of_symbol++;
    }

    leSizeOfStringTable = htole32(size_of_symbol + 4);

    /* open output file */
    change_dir(conf->outdir);
    fpOut = open_file(conf->outfile, "wb");

    /* header */
    WRITE_BUF(coff_header, fpOut);
    WRITE_BUF(section_table, fpOut);

    /* file data */
    while ((nread = fread(&buffer, 1, sizeof(buffer), fpIn)) != 0) {
        write_data(&buffer, nread, fpOut);
    }

    /* null byte, symbol table, string table */
    WRITE_BUF(nullbyte, fpOut);
    WRITE_BUF(symbol_table, fpOut);
    WRITE_BUF(leSizeOfStringTable, fpOut);

    if (mangle) {
        write_data("_", 1, fpOut);
    }
    write_data(conf->symbol, size_of_symbol, fpOut);

    fclose(fpOut);
    fclose(fpIn);
}

/* takes input files directory as optional argument */
int main(int argc, char **argv)
{
    char *cwd, *p;
    struct cfg conf = {0};

    conf.machine[0] = MDEF[0];
    conf.machine[1] = MDEF[1];

    /* parse arguments */
    for (int i = 1; i < argc; i++) {
        p = argv[i];

        if ((*p != '/' && *p != '-') || strlen(p) == 1) {
            err_exit("unknown option: ", argv[i]);
        }

        p++;

        switch (tolower(*p))
        {
        case '?':
        case 'h':
        case 'i':
        case 'm':
            break;
        default:
            err_exit("unknown option: ", argv[i]);
        }

        if (STREQ(p, "help") || STREQ(p, "?")) {
            printf("%s [OPTIONS]\n"
                "  /?\n"
                "  /HELP\n"
                "  /I:{directory}\n"
                "  /MACHINE:{ARM|ARM64|X64|X86|NONE}\n", argv[0]);
            return 0;
        }
        else if (STREQ(p, "i") || STREQ(p, "i:") ||
                STREQ(p, "machine") || STREQ(p, "machine:"))
        {
            err_exit("missing argument: ", argv[i]);
        }
        else if (STRBEG(p, "i:")) {
            conf.indir = p + 2;
        }
        else if (STRBEG(p, "machine:")) {
            p += 8;

            if (STREQ("X86", p)) {
                conf.machine[0] = mx86[0];
                conf.machine[1] = mx86[1];
            } else if (STREQ("X64", p)) {
                conf.machine[0] = mx64[0];
                conf.machine[1] = mx64[1];
            } else if (STREQ("ARM", p)) {
                conf.machine[0] = marmnt[0];
                conf.machine[1] = marmnt[1];
            } else if (STREQ("ARM64", p)) {
                conf.machine[0] = marm64[0];
                conf.machine[1] = marm64[1];
            } else if (STREQ("NONE", p)) {
                conf.machine[0] = munknown[0];
                conf.machine[1] = munknown[1];
            } else {
                err_exit("unknown argument for /MACHINE: ", p);
            }
        } else {
            err_exit("unknown option: ", argv[i]);
        }
    }

    /* CWD */
    if ((cwd = getcwd(NULL, 0)) == NULL) {
        perror("getcwd()");
        exit(1);
    }

    conf.outdir = cwd;

#define SAVE_FILE(INPUT, SYMBOL) \
    conf.infile = INPUT; \
    conf.outfile = "templ_" INPUT ".obj"; \
    conf.symbol = SYMBOL; \
    save_to_coff(&conf)

    SAVE_FILE("filename_macros.h", "filename_macros_data");
    SAVE_FILE("license.h",         "license_data");
    SAVE_FILE("common.h",          "common_header_data");
    SAVE_FILE("c.h",               "c_header_data");
    SAVE_FILE("c.c",               "c_body_data");
    SAVE_FILE("cxx.hpp",           "cxx_header_data");
    SAVE_FILE("cxx.cpp",           "cxx_body_data");
    SAVE_FILE("minimal.h",         "min_c_header_data");
    SAVE_FILE("minimal_cxxeh.hpp", "min_cxx_header_data");

    free(cwd);

    remove("template.lib");

    return system(WINE "lib -nologo -out:template.lib templ_*.obj");
}

