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
#include <time.h>

#include "template.h"
#include "gendlopen.hpp"

#define RANGE(c, beg, end)  (c >= beg && c <= end)


/* convert input string to be used as prefixes or header guards */

static std::string convert_to_upper(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (RANGE(c, 'A', 'Z') || RANGE(c, '0', '9')) {
            out += c;
        } else if (RANGE(c, 'a', 'z')) {
            out += c - ('a'-'A');
        } else {
            out += '_';
        }
    }

    if (out.back() != '_') {
        out += '_';
    }

    return out;
}

static std::string convert_to_lower(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (RANGE(c, 'a', 'z') || RANGE(c, '0', '9')) {
            out += c;
        } else if (RANGE(c, 'A', 'Z')) {
            out += c + ('a'-'A');
        } else {
            out += '_';
        }
    }

    if (out.back() != '_') {
        out += '_';
    }

    return out;
}

/* create a note to put at the beginning of the output */
static std::string create_note(int &argc, char ** &argv)
{
    std::stringstream str;

    time_t t = ::time(NULL);
    tm tm = *::localtime(&t);

    str << "// Do not edit this file!\n";
    str << "// It was created by gendlopen on "
        << std::put_time(&tm, "%c %Z") << '\n';
    str << "// using the following flags:\n";
    str << "//";

    for (int i=1; i < argc; i++) {
        str << ' ' << argv[i];
    }

    str << "\n\n" << license_data << '\n';

    return str.str();
}

static void format_lib_name(std::string &lib, const std::string &prefix)
{
    size_t pos = lib.find(':');

    if (pos != std::string::npos) {
        /* create macro: "name:0" -> GDO_LIB(name,0) */
        lib.replace(pos, 1, 1, ',');
        lib.insert(0, "LIB(");
        lib.insert(0, prefix);
        lib += ')';
    } else {
        /* add hyphens if needed */

        if (lib.back() != '"') {
            lib += '"';
        }

        if (lib.front() != '"') {
            lib.insert(0, 1, '"');
        }
    }
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

    std::string header_data = common_header_data;
    std::string body_data;
    bool is_c = true;

    switch (m_out)
    {
    case output::cxx:
        is_c = false;

        /* header + wrap + header2 */
        header_data += cxx_header_data;

        if (!m_skip_parameter_names) {
            header_data += cxx_wrap_data;
        }
        header_data += cxx_header_data2;
        break;

    case output::minimal:
        /* minimal header */
        header_data += minimal_header_data;
        break;

    case output::c:
        const char *wrap_data = m_skip_parameter_names ? "" : c_wrap_data;
        header_data += c_header_data;

        if (m_separate) {
            /* header / body + wrap */
            body_data = c_body_data;
            body_data += wrap_data;
        } else {
            /* header + body + wrap */
            header_data += c_body_data;
            header_data += wrap_data;
        }
        break;
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

    /* create "#include" filename and header guard */
    std::string header_name;

    if (use_stdout) {
        header_name = name + ".h";

        if (!is_c) {
            header_name += "pp";
        }
    } else {
        header_name = ofhdr.filename().string();
    }

    m_guard = convert_to_upper(header_name);
    m_guard.insert(0, 1, '_');

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

    /* header guards */
    out << "#ifndef " << m_guard << '\n';
    out << "#define " << m_guard << "\n\n";

    /* default library name */
    if (!m_default_lib.empty()) {
        format_lib_name(m_default_lib, m_name_upper);
        out << "#ifndef " << m_name_upper << "DEFAULT_LIB\n";
        out << "#define " << m_name_upper << "DEFAULT_LIB " << m_default_lib << '\n';
        out << "#endif\n\n";
    }

    /* extra includes and defines (might be empty) */
    out << m_extra_code;

    /* extern "C" */
    if (is_c) {
        out << "#ifdef __cplusplus\n";
        out << "extern \"C\" {\n";
        out << "#endif\n\n";
    }

    /* header code */
    out << parse(header_data, proto, objs);

    /* extern "C" end */
    if (is_c) {
        out << '\n';
        out << "#ifdef __cplusplus\n";
        out << "} /* extern \"C\" */\n";
        out << "#endif\n";
    }

    /* header guard */
    out << '\n';
    out << "#endif //" << m_guard << '\n';

    /* body data */
    if (m_separate) {
        out_body << note;
        out_body << "#include \"" << header_name << "\"\n\n";
        out_body << parse(body_data, proto, objs);
    }


    /* STDOUT */
    if (use_stdout) {
        std::cout << out.str() << std::flush;
        return;
    }


    /* open file streams */
    std::ofstream ofs, ofs_body;

    if (!open_fstream(ofs, ofhdr.string())) {
        std::exit(1);
    }

    if (m_separate && !open_fstream(ofs_body, ofbody.string())) {
        std::exit(1);
    }

    /* write to header file */
    ofs << out.str();
    std::cout << "saved to file: " << ofhdr << std::endl;

    /* write to body file */
    if (ofs_body.is_open()) {
        ofs_body << out_body.str();
        std::cout << "saved to file: " << ofbody << std::endl;
    }
}

