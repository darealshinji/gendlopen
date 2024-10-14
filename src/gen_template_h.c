/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2024 Carsten Janssen
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

#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
# include <direct.h>
#else
# include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 16


static void hexdump(const char *in, const char *varName, FILE *fpOut)
{
    FILE *fp = NULL;
    unsigned char buf[BUF_SIZE];

    if ((fp = fopen(in, "rb")) == NULL) {
        perror("fopen()");
        fprintf(stderr, "-> %s\n", in);
        exit(1);
    }

    fprintf(fpOut, "/* %s */\n", in);
    fprintf(fpOut, "static const char %s[] =\n{", varName);

    while (feof(fp) == 0)
    {
        size_t items = fread(buf, 1, BUF_SIZE, fp);

        if (ferror(fp) != 0) {
            perror("fread()");
            fprintf(stderr, "(%s)\n", in);
            exit(1);
        }

        fprintf(fpOut, "%s", "\n  ");

        for (size_t i = 0; i < items; ++i) {
            switch (buf[i]) {
            case '\t':
                fprintf(fpOut, "'%s',", "\\t");
                break;
            case '\n':
                fprintf(fpOut, "'%s',", "\\n");
                break;
            case '\'':
                fprintf(fpOut, "'%s',", "\\'");
                break;
            case '\\':
                fprintf(fpOut, "'%s',", "\\\\");
                break;
            default:
                if (buf[i] < 0x20 || buf[i] > 0x7E) {
                    fprintf(fpOut, "0x%02x,", buf[i]);
                } else {
                    fprintf(fpOut, "'%c',", buf[i]);
                }
                break;
            }
        }
    }

    fprintf(fpOut, "%s", "0x0\n};\n\n");
    fclose(fp);
}

/* optional argument provided: path to source directory,
 * used for out-of-tree builds */
int main(int argc, char **argv)
{
    const char *out = "template.h";
    FILE *fp = fopen(out, "wb");

    if (!fp) {
        perror("fopen()");
        fprintf(stderr, "(%s)\n", out);
        exit(1);
    }

    if (argc > 1 && chdir(argv[1]) == -1) {
        perror("chdir()");
        fprintf(stderr, "(%s)\n", argv[1]);
        exit(1);
    }

    fprintf(fp, "%s\n", "#ifndef _TEMPLATE_H_");
    fprintf(fp, "%s\n", "#define _TEMPLATE_H_\n");

    hexdump("filename_macros.h", "filename_macros_data", fp);
    hexdump("license.h",         "license_data",         fp);
    hexdump("common.h",          "common_header_data",   fp);
    hexdump("c.h",               "c_header_data",        fp);
    hexdump("c.c",               "c_body_data",          fp);
    hexdump("cxx.hpp",           "cxx_header_data",      fp);
    hexdump("cxx.cpp",           "cxx_body_data",        fp);
    hexdump("minimal.h",         "min_c_header_data",    fp);
    hexdump("minimal_cxxeh.hpp", "min_cxx_header_data",  fp);

    fprintf(fp, "%s\n", "#endif //_TEMPLATE_H_");
    fclose(fp);

    return 0;
}
