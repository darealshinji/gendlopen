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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <ctype.h>

#include "gendlopen.hpp"
#include "template.h"


/* convert input string to be used as prefixes or header guards */
static std::string convert_name(const char *prefix, const std::string &in, int (*to_case)(int))
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
static bool open_fstream(std::ofstream &ofs, const std::string &ofile, bool force_overwrite)
{
    if (!force_overwrite) {
        ofs.open(ofile.c_str(), std::ios::in);

        if (ofs.is_open()) {
            std::cerr << "error: file already exists: " << ofile << std::endl;
            return false;
        }
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
    /* read data */
    if (!tokenize(ifile)) std::exit(1);

    /* output filename */
    std::filesystem::path ofhdr;
    bool use_stdout = (ofile == "-");

    if (use_stdout == false) {
        ofhdr = ofile;

        if (m_cxx) {
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
        header_name = name + ".h";
        if (m_cxx) header_name += "pp";
    } else {
        header_name = ofhdr.filename().string();
    }

    m_guard = convert_name("_", header_name, toupper);

    /* name used on variables and macros */
    m_name_upper = convert_name("", name, toupper);
    m_name_lower = convert_name("", name, tolower);

    /* create output */

    auto header_data = m_cxx ? template_cxx_header_data : template_c_header_data;
    auto body_data = m_cxx ? template_cxx_body_data : template_c_body_data;

    if (use_stdout) {
        /* STDOUT */
        put_header_guards<std::ostream>(std::cout, header_data, body_data, license_data);
    } else {
        std::ofstream ofs;

        if (!open_fstream(ofs, ofhdr.string().c_str(), m_force)) {
            std::exit(1);
        }

        if (m_separate) {
            /* separate files */
            std::ofstream ofs_body;
            auto ofbody = ofhdr;
            ofbody.replace_extension(m_cxx ? ".cpp" : ".c");

            if (!open_fstream(ofs_body, ofbody.string().c_str(), m_force)) {
                std::exit(1);
            }

            /* header */
            put_header_guards(ofs, header_data, "", license_data);
            std::cout << "saved to file: " << ofhdr << std::endl;

            /* body */
            ofs_body << license_data;
            ofs_body << "\n#include \"" << header_name << "\"\n\n";
            ofs_body << parse(body_data);
            std::cout << "saved to file: " << ofbody << std::endl;
        } else {
            /* single file */
            put_header_guards(ofs, header_data, body_data, license_data);
            std::cout << "saved to file: " << ofhdr << std::endl;
        }
    }
}

