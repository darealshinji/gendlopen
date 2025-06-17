/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

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

#ifdef _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
# endif
# ifndef _CRT_NONSTDC_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
# endif
# include <direct.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void dump(FILE *fpOut, const char *in_dir, const char *in_file, const char *varName,
    bool line_directive, bool print_line_count)
{
    int c = 0;
    bool multi_line = false;
    bool new_line = true;
    size_t count = 0;
    size_t total_line_count = 0;
    FILE *fp;

    const char * const true_false[2] = { "false", "true" };
    int percent = 0;

    /* concat path */
    const size_t len = strlen(in_dir) + strlen(in_file) + 2;
    char *buf = malloc(len);
    snprintf(buf, len, "%s/%s", in_dir, in_file);

    /* open input file */
    if ((fp = fopen(buf, "rb")) == NULL) {
        perror("fopen()");
        fprintf(stderr, "-> %s\n", buf);
        exit(1);
    }

    free(buf);

    /* write output */
    fprintf(fpOut, "/* %s */\n", in_file);
    fprintf(fpOut, "#define FILENAME_%s \"%s\"\n", varName, in_file);
    fprintf(fpOut, "static const template_t %s[] = {\n", varName);

    if (line_directive) {
        fprintf(fpOut, "  { \"#line 1 \\\"<built-in>/%s\\\"\", false, 1 },\n", in_file);
        total_line_count++;
    }

#define READ_NEXT() (c = fgetc(fp))

    while (READ_NEXT() != EOF)
    {
        if (new_line) {
            fprintf(fpOut, "%s", "  { \"");
            new_line = false;
        }

        switch (c)
        {
        case '\t':
            fprintf(fpOut, "%s", "\\t");
            break;
        case '"':
            fprintf(fpOut, "%s", "\\\"");
            break;
        case '\\':
            fprintf(fpOut, "%s", "\\\\");
            break;

        case '%':
            fprintf(fpOut, "%c", '%');
            percent = 1;
            break;

        case '\r':
            /* ignore \r only in line endings */
            if (READ_NEXT() != '\n') {
                fprintf(fpOut, "%s", "\\r");
                ungetc(c, fp);
                break;
            }
            /* FALLTHROUGH */

        case '\n':
            count++;
            fprintf(fpOut, "\", %s, %zd },\n", true_false[percent], count);
            total_line_count += count;
            count = percent = 0;
            new_line = true;
            multi_line = false;
            break;

        case '@':
            /* concatenate lines ending on »@\n« */
            if (READ_NEXT() == '\r' && READ_NEXT() != '\n') {
                /* not a »\r\n« line ending */
                fprintf(fpOut, "%s", "@\r");
                ungetc(c, fp);
                break;
            }

            if (c == '\n' || c == EOF) {
                if (!multi_line) {
                    fprintf(fpOut, "%s", "\\n\" /* multiline entry */\n");
                    multi_line = true;
                } else {
                    fprintf(fpOut, "%s", "\\n\"\n");
                }
                fprintf(fpOut, "%s", "    \"");
                count++;
            } else {
                fprintf(fpOut, "%c", '@');
                ungetc(c, fp);
            }
            break;

        default:
            if (c < ' ' || c > '~') {
                fprintf(fpOut, "\\x%02X", (unsigned char)c);
            } else {
                fprintf(fpOut, "%c", (char)c);
            }
            break;
        }
    }

    if (!new_line) {
        count++;
        fprintf(fpOut, "\", %s, %zd },\n", true_false[percent], count);
        total_line_count += count;
    }

    fprintf(fpOut, "%s", "  { NULL, 0, 0 }\n");
    fprintf(fpOut, "%s", "};\n");

    if (!print_line_count) {
        fprintf(fpOut, "%s", "//");
    }
    fprintf(fpOut, "static const size_t %s_lines = %zd;\n\n", varName, total_line_count);

    fclose(fp);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <INPUT_DIR> <OUTFILE>\n", argv[0]);
        return 1;
    }

    /* input directory */
    const char *d = argv[1];

    /* open output file for writing */
    FILE *fp = fopen(argv[2], "wb");

    if (!fp) {
        perror("fopen()");
        fprintf(stderr, "[%s]\n", argv[2]);
        return 1;
    }

    fprintf(fp, "%s\n",
        "/* this file was automatically generated; do not edit! */\n"
        "\n"
        "#pragma once\n");

/* fpOut, in_dir, in_file,           varName,           line_directive, print_line_count */
    dump(fp, d, "license.h",         "license",         false, true);
    dump(fp, d, "filename_macros.h", "filename_macros", true,  true);
    dump(fp, d, "common.h",          "common_header",   true,  false);
    dump(fp, d, "c.h",               "c_header",        true,  false);
    dump(fp, d, "c.c",               "c_body",          true,  false);
    dump(fp, d, "cxx.hpp",           "cxx_header",      true,  false);
    dump(fp, d, "cxx.cpp",           "cxx_body",        true,  false);
    dump(fp, d, "minimal.h",         "min_c_header",    true,  false);
    dump(fp, d, "minimal_cxxeh.hpp", "min_cxx_header",  true,  false);
    dump(fp, d, "plugin.h",          "plugin_header",   true,  false);
    dump(fp, d, "plugin.c",          "plugin_body",     true,  false);

    fclose(fp);

    return 0;
}
