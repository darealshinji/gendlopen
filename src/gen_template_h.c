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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static FILE *open_file(const char *name)
{
    FILE *fp = fopen(name, "rb");

    if (!fp) {
        perror("fopen()");
        fprintf(stderr, "-> %s\n", name);
        exit(1);
    }

    return fp;
}

static void textdump(const char *in, const char *varName, FILE *fpOut)
{
    int c = 0;
    bool multi_line = false;
    bool new_line = true;
    int count = 0;
    int percent = 0;

    FILE *fp = open_file(in);

    fprintf(fpOut, "/* %s */\n", in);
    fprintf(fpOut, "static const template_t %s[] = {\n", varName);

    while ((c = fgetc(fp)) != EOF)
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

        case '\n':
            fprintf(fpOut, "\", %d, %d },\n", percent, ++count);
            count = percent = 0;
            new_line = true;
            multi_line = false;
            break;

        case '@':
            /* concatenate lines ending on »@\n« */
            if ((c = fgetc(fp)) == '\n' || c == EOF) {
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
        fprintf(fpOut, "\", %d, %d },\n", percent, ++count);
    }

    fprintf(fpOut, "%s", "  { NULL, 0, 0 }\n");
    fprintf(fpOut, "%s", "};\n\n");
    fclose(fp);
}

static void textdump_simple(const char *in, const char *varName, FILE *fpOut)
{
    int c = 0;
    bool new_line = true;
    int lines = 0;

    FILE *fp = open_file(in);

    fprintf(fpOut, "/* %s */\n", in);
    fprintf(fpOut, "static const char *%s =\n", varName);

    while ((c = fgetc(fp)) != EOF)
    {
        if (new_line) {
            fprintf(fpOut, "%s", "  \"");
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

        case '\n':
            fprintf(fpOut, "%s", "\\n\"\n");
            new_line = true;
            lines++;
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
        fprintf(fpOut, "%s", "\\n\"\n");
        lines++;
    }

    fprintf(fpOut, "%s", "  /**/;\n");
    fprintf(fpOut, "static const int %s_linecount = %d;\n\n", varName, lines);

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

    fprintf(fp, "%s\n",
        "/* this file was automatically generated; do not edit! */\n"
        "\n"
        "#ifndef TEMPLATE_H\n"
        "#define TEMPLATE_H\n");

    textdump_simple("filename_macros.h", "filename_macros", fp);
    textdump_simple("license.h",         "license",         fp);
    textdump("common.h",          "common_header",   fp);
    textdump("c.h",               "c_header",        fp);
    textdump("c.c",               "c_body",          fp);
    textdump("cxx.hpp",           "cxx_header",      fp);
    textdump("cxx.cpp",           "cxx_body",        fp);
    textdump("minimal.h",         "min_c_header",    fp);
    textdump("minimal_cxxeh.hpp", "min_cxx_header",  fp);

    fprintf(fp, "%s\n", "#endif /* TEMPLATE_H */");
    fclose(fp);

    return 0;
}
