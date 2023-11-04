/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2023 djcj@gmx.de

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
*/

#ifdef _WIN32
    #ifdef _MSC_VER
        #define _CRT_SECURE_NO_WARNINGS
    #endif
    #include <direct.h>
#else
    #include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strlen("0xff,")*16 == 80 */
#define BUF_SIZE 16


static void hexdump(const char *in, const char *varName, FILE *fpOut)
{
    FILE *fp = NULL;
    unsigned char buf[BUF_SIZE];

    if ((fp = fopen(in, "rb")) == NULL) {
        perror("fopen()");
        fprintf(stderr, "(%s)\n", in);
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

        fprintf(fpOut, "%c", '\n');

        for (size_t i = 0; i < items; ++i) {
            fprintf(fpOut, "0x%02x,", buf[i]);
        }
    }

    fprintf(fpOut, "\n0x00\n};\n\n");

    if (fp) {
        fclose(fp);
    }
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

    hexdump("license.h",          "license_data",                 fp);
    hexdump("template_common.h",  "template_common_header_data",  fp);
    hexdump("template_c.h",       "template_c_header_data",       fp);
    hexdump("template_c.c",       "template_c_body_data",         fp);
    hexdump("template_cxx.hpp",   "template_cxx_header_data",     fp);
    hexdump("template_cxx.cpp",   "template_cxx_body_data",       fp);
    hexdump("template_minimal.h", "template_minimal_header_data", fp);

    //printf("data written to `%s'\n", out);
    fclose(fp);

    return 0;
}
