/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023-2024 djcj@gmx.de
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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <ctype.h>

#include "gendlopen.hpp"
#include "template.h"


/* convert input string to be used as prefixes or header guards */
inline static
std::string convert_name(const char *prefix, const std::string &in, int (*to_case)(int))
{
    std::string out = prefix;

    for (const char &c : in) {
        out += isalnum(c) ? to_case(c) : '_';
    }

    if (out.back() != '_') {
        out += '_';
    }

    return out;
}

/* open file for writing */
bool gendlopen::open_fstream(std::ofstream &ofs, const std::string &ofile)
{
    if (!m_force) {
        ofs.open(ofile.c_str(), std::ios::in);

        if (ofs.is_open()) {
            std::cerr << "error: file already exists: " << ofile << std::endl;
            return false;
        }

        ofs.close();
    }

    ofs.open(ofile.c_str(), std::ios::out | std::ios::trunc);

    if (!ofs.is_open()) {
        std::cerr << "error: failed to open file for writing: " << ofile << std::endl;
        return false;
    }

    return true;
}

void gendlopen::generate(
    const std::string &ifile,
    const std::string &ofile,
    const std::string &name)
{
    tokenize tok;

    /* read data */
    if (!tok.tokenize_file(ifile)) {
        std::exit(1);
    }

    /* output filename */
    std::filesystem::path ofhdr;
    bool use_stdout = (ofile == "-");

    if (use_stdout == false) {
        ofhdr = ofile;

        if (m_out == output::cxx) {
            /* C++ */
            auto ext = ofhdr.extension();

            if (ext != ".h" && ext != ".hpp" && ext != ".hxx") {
                ofhdr.replace_extension(".hpp");
            }
        } else {
            /* C */
            ofhdr.replace_extension(".h");
        }
    }

    /* "#include" filename and header guard */
    std::string header_name;

    if (use_stdout) {
        if (m_out == output::cxx) {
            header_name = name + ".hpp";
        } else {
            header_name = name + ".h";
        }
    } else {
        header_name = ofhdr.filename().string();
    }

    m_guard = convert_name("_", header_name, toupper);

    /* name used on variables and macros */
    m_name_upper = convert_name("", name, toupper);
    m_name_lower = convert_name("", name, tolower);

    /* create output */

    auto proto = tok.prototypes();
    auto objs = tok.objects();

    const char *header_data = c_header_data;
    const char *body_data = c_body_data;

    switch (m_out)
    {
    case output::cxx:
        header_data = cxx_header_data;
        body_data = "";
        break;
    case output::minimal:
        header_data = minimal_header_data;
        body_data = "";
        break;
    case output::c:
        break;
    }

    if (use_stdout) {
        /* STDOUT */
        put_header_guards<std::ostream>(std::cout, header_data, body_data, license_data, proto, objs);
    } else {
        std::ofstream ofs;

        if (!open_fstream(ofs, ofhdr.string())) {
            std::exit(1);
        }

        if (m_separate) {
            /* separate files */
            std::ofstream ofs_body;
            auto ofbody = ofhdr;
            ofbody.replace_extension(m_out == output::cxx ? ".cpp" : ".c");

            if (!open_fstream(ofs_body, ofbody.string())) {
                std::exit(1);
            }

            /* header */
            put_header_guards(ofs, header_data, "", license_data, proto, objs);
            std::cout << "saved to file: " << ofhdr << std::endl;

            /* body */
            ofs_body << license_data;
            ofs_body << "\n#include \"" << header_name << "\"\n\n";
            ofs_body << parse(body_data, proto, objs);
            std::cout << "saved to file: " << ofbody << std::endl;
        } else {
            /* single file */
            put_header_guards(ofs, header_data, body_data, license_data, proto, objs);
            std::cout << "saved to file: " << ofhdr << std::endl;
        }
    }
}

