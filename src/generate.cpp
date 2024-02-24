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

/**
 * Generate the output data (STDOUT or save to file).
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <ctime>

#include "template.h"
#include "gendlopen.hpp"

using common::range;
using common::replace_string;


/* convert input string to be used as prefixes or header guards */

static std::string convert_to_upper(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (range(c, 'a','z')) {
            out += c - ('a'-'A');
        } else if (range(c, 'A','Z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

static std::string convert_to_lower(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (range(c, 'A','Z')) {
            out += c + ('a'-'A');
        } else if (range(c, 'a','z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

/* create a note to put at the beginning of the output */
static std::string create_note(int &argc, char ** &argv)
{
    std::stringstream str;
    std::string line = "//";

    time_t t = std::time(nullptr);
    tm *tmb = std::localtime(&t);

    /* use C locale for std::put_time() */
    str.imbue(std::locale("C"));

    str << "// Do not edit this file!\n";
    str << "// It was created by gendlopen on " << std::put_time(tmb, "%c %Z") << '\n';
    str << "// using the following flags:\n\n";

    for (int i=1; i < argc; i++) {
        /* split long lines */
        if ((line.size() + std::strlen(argv[i]) + 3) >= 80) {
            str << line << '\n';
            line = "//";
        }
        line += ' ';
        line += argv[i];
    }

    str << line << "\n\n";
    str << license_data << '\n';

    return str.str();
}

/* create "#define" lines */
static std::string format_definitions(const std::vector<std::string> &list)
{
    std::stringstream out;

    for (auto e : list) {
        size_t pos = e.find('=');

        if (pos == std::string::npos) {
            /* no value given */
            out << "#ifndef " << e << '\n';
        } else {
            /* remove value from "#ifndef" */
            out << "#ifndef " << e.substr(0, pos) << '\n';

            /* replace '=' with space */
            e.replace(pos, 1, 1, ' ');
        }

        out << "#define " << e << '\n';
        out << "#endif\n\n";
    }

    return out.str();
}

/* create "#include" lines */
static std::string format_includes(const std::vector<std::string> &list)
{
    std::stringstream out;

    for (auto e : list) {
        if (e.front() == '<' && e.back() == '>') {
            /* <foo.h> */
            out << "#include " << e << '\n';
        } else {
            /* "foo.h" */
            const char * const q1 = (e.front() == '"') ? "" : "\"";
            const char * const q2 = (e.back() == '"') ? "" : "\"";
            out << "#include " << q1 << e << q2 << '\n';
        }
    }

    out << '\n';

    return out.str();
}

/* create the default library name macro */
static std::string format_library_name(const std::string &name, const std::string &prefix)
{
    std::string out =
        "#ifndef {pfx}_DEFAULT_LIB\n"
        "#define {pfx}_DEFAULT_LIB {lib}\n"
        "#endif\n\n";

    std::stringstream lib;
    const size_t pos = name.find(':');

    if (pos != std::string::npos) {
        /* create macro: "name:0" -> GDO_LIB(name,0) */
        lib << prefix << "_LIB(" << name.substr(0, pos) << ','
            << name.substr(pos+1) << ')';
    } else {
        /* use the library name as is; add hyphens if needed */
        if (name.front() != '"') {
            lib << '"';
        }

        lib << name;

        if (name.back() != '"') {
            lib << '"';
        }
    }

    replace_string("{pfx}", prefix, out);
    replace_string("{lib}", lib.str(), out);

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
    if (!tok.tokenize_file(ifile, m_skip_parameter_names)) {
        std::exit(1);
    }

    /* check output type */

    const char *wrap_data = "\n"
        "#ifdef GDO_WRAP_FUNCTIONS\n"
        "#error \"GDO_WRAP_FUNCTIONS\" defined but wrapped functions were disabled by \"--skip-parameter-names\"\n"
        "#endif\n";

    std::string header_data = common_header_data;
    std::string body_data;
    bool is_c = true;

    switch (m_out)
    {
    case output::cxx: {
            /* header + wrap + header2 */
            header_data += cxx_header_data;

            if (!m_skip_parameter_names) {
                wrap_data = cxx_wrap_data;
            }

            header_data += wrap_data;
            header_data += cxx_header_data2;
            is_c = false;
        }
        break;

    case output::c: {
            header_data += c_header_data;

            if (!m_skip_parameter_names) {
                wrap_data = c_wrap_data;
            }

            if (m_separate) {
                /* header / body + wrap */
                body_data = c_body_data;
                body_data += wrap_data;
            } else {
                /* header + body + wrap */
                header_data += c_body_data;
                header_data += wrap_data;
            }
        }
        break;

    case output::minimal:
        /* minimal header */
        header_data += minimal_header_data;
        break;

//  default:
//      ASSERT();
    }

    /* output filename */
    std::filesystem::path ofhdr(ofile);
    auto ofbody = ofhdr;
    bool use_stdout = (ofile == "-");

    if (use_stdout) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
    if (m_separate) {
        if (is_c) {
            /* C */
            ofhdr.replace_extension(".h");
            ofbody.replace_extension(".c");
        } else {
            /* C++ */
            auto ext = ofhdr.extension();

            if (ext.empty() || (ext != ".h" && ext != ".H" && ext != ".hpp" && ext != ".hxx")) {
                ofhdr.replace_extension(".hpp");
            }
            ofbody.replace_extension(".cpp");
        }
    }

    /* create header filename and header guard */
    std::string header_name;

    if (use_stdout) {
        header_name = name + ".h";

        if (!is_c) {
            header_name += "pp";
        }
    } else {
        header_name = ofhdr.filename().string();
    }

    auto header_guard = convert_to_upper(header_name);

    /* name used on variables and macros */
    m_name_upper = convert_to_upper(name);
    m_name_lower = convert_to_lower(name);


    /* header data */

    auto proto = tok.prototypes();
    auto objs = tok.objects();
    auto note = create_note(*m_argc, *m_argv);
    std::stringstream out, out_body;

    /* notification + license text */
    out << note;

    /* header guard begin */
    out << "#ifndef _" << header_guard << "_\n";
    out << "#define _" << header_guard << "_\n\n";

    /* extra definitions */
    if (!m_definitions.empty()) {
        out << format_definitions(m_definitions);
    }

    /* default library name */
    if (!m_default_lib.empty()) {
        out << format_library_name(m_default_lib, m_name_upper);
    }

    /* extra includes */
    if (!m_includes.empty()) {
        out << format_includes(m_includes);
    }

    /* extern "C" begin */
    if (is_c) {
        out << "#ifdef __cplusplus\n";
        out << "extern \"C\" {\n";
        out << "#endif\n\n";
    }

    /* header code from template */
    out << parse(header_data, proto, objs);

    /* extern "C" end */
    if (is_c) {
        out << '\n';
        out << "#ifdef __cplusplus\n";
        out << "} /* extern \"C\" */\n";
        out << "#endif\n";
    }

    /* header guard end */
    out << '\n';
    out << "#endif //_" << header_guard << "_\n";

    /* body data */
    if (m_separate) {
        out_body << note;
        out_body << "#include \"" << header_name << "\"\n\n";
        out_body << parse(body_data, proto, objs);
    }

    /* print to STDOUT (m_separate is always set false in this case) */
    if (use_stdout) {
        std::cout << out.str() << std::flush;
        return;
    }

    /* write to file(s) */

    std::ofstream ofs, ofs_body;

    if (!open_fstream(ofs, ofhdr.string())) {
        std::exit(1);
    }

    if (m_separate && !open_fstream(ofs_body, ofbody.string())) {
        std::exit(1);
    }

    ofs << out.str();
    std::cout << "saved to file: " << ofhdr << std::endl;

    if (ofs_body.is_open()) {
        ofs_body << out_body.str();
        std::cout << "saved to file: " << ofbody << std::endl;
    }
}

