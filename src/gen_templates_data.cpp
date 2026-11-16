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

#include <cctype>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>


static void dump(std::ofstream &ofs, const char *in_dir, const char *in_file, const char *varName)
{
    std::string path = in_dir;
    path += '/';
    path += in_file;

    std::ifstream ifs(path.c_str());

    if (!ifs.is_open()) {
        std::cerr << "failed to open file for reading: " << path << std::endl;
        std::exit(1);
    }

#define BB  "\\" /* backslash */
#define QQ  "\"" /* double quotes */

    ofs << '\n';
    ofs << "/* " << in_file << " */\n";
    ofs << "const char *" << varName
        << " = \"#line 1 " BB QQ "<built-in>/" << in_file << BB QQ BB "n\"\n";
    ofs << QQ;

    char c;
    int cnt = 1;

    while (ifs.get(c)) {
        if (std::isprint(c) && c != '"' && c != '\\') {
            /* raw character */
            ofs << c;
            cnt++;
        } else {
            /* hex sequence */
            ofs << QQ " " QQ BB "x";
            ofs << std::setw(2) << std::setfill('0')
                << std::hex << static_cast<short>(c);
            ofs << QQ " " QQ;
            cnt += 10;
        }

        if (cnt > 78) {
            /* line break */
            ofs << QQ "\n";
            ofs << QQ;
            cnt = 1;
        }
    }

    ofs << QQ ";\n";
}


int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <INPUT_DIR> <OUTFILE>" << std::endl;
        return 1;
    }

    const char *indir = argv[1];
    const char *outfile = argv[2];

    std::ofstream ofs(outfile);

    if (!ofs.is_open()) {
        std::cerr << "failed to open file for writing: " << outfile << std::endl;
        return 1;
    }

    ofs << "/* this file was automatically generated; do not edit! */\n";
    ofs << "#include \"types.hpp\"\n";
    ofs << '\n';
    ofs << "namespace templates {\n";

#define TEMPLATE(FILE, VAR) dump(ofs, indir, #FILE, #VAR);
#include "list.h"

    ofs << '\n';
    ofs << "} /* namespace templates */\n";

    return 0;
}
