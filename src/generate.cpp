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

/**
 * Generate the output data (STDOUT or save to file).
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <ctime>

#include "template.h"
#include "gendlopen.hpp"


namespace /* anonymous */
{

const char * const extern_c_begin =
    "#ifdef __cplusplus\n"
    "extern \"C\" {\n"
    "#endif\n\n";

const char * const extern_c_end =
    "\n"
    "#ifdef __cplusplus\n"
    "} /* extern \"C\" */\n"
    "#endif\n";

/* create a note to put at the beginning of the output */
std::string create_note(int &argc, char ** &argv)
{
    auto localtime_wrapper = [] (const time_t *timep, struct tm *tm) -> struct tm *
    {
#ifdef _WIN32
        return (::localtime_s(tm, timep) == 0) ? tm : nullptr;
#else
        return ::localtime_r(timep, tm);
#endif
    };

    std::string line = "//";
    std::stringstream out;

    out << "// Do not edit this file!\n";
    out << "// It was created by gendlopen ";

    /* print date (C locale) */
    struct tm tm = {};
    time_t t = std::time(nullptr);

    if (localtime_wrapper(&t, &tm))
    {
#if !defined(_WIN32)
        out.imbue(std::locale("C"));
#endif
        out << "on " << std::put_time(&tm, "%c %Z") << '\n';
        out << "// ";
    }

    out << "using the following flags:\n\n";

    /* print used flags */

    for (int i=1; i < argc; i++) {
        /* split long lines */
        if ((line.size() + std::strlen(argv[i]) + 3) >= 80) {
            out << line << '\n';
            line = "//";
        }
        line += ' ';
        line += argv[i];
    }

    out << line << "\n\n";
    out << license_data << '\n';

    return out.str();
}

/* define default library name */
void print_default_libname(
    cio::ofstream &out,
    const std::string &pfx,
    const std::string &lib_a,
    const std::string &lib_w)
{
    if (!lib_a.empty() && !lib_w.empty()) {
        out << "/* default library */\n";
        out << "#ifndef " << pfx << "_DEFAULT_LIBA\n";
        out << "#define " << pfx << "_DEFAULT_LIBA " << lib_a << "\n";
        out << "#endif\n";
        out << "#ifndef " << pfx << "_DEFAULT_LIBW\n";
        out << "#define " << pfx << "_DEFAULT_LIBW " << lib_w << "\n";
        out << "#endif\n\n";
    }
}

/* extra defines */
void print_extra_defines(cio::ofstream &out, const std::string &defs)
{
    if (!defs.empty()) {
        out << "/* extra defines */\n";
        out << defs;
        out << '\n';
    }
}

/* extra includes */
void print_includes(cio::ofstream &out, const vstring_t &incs)
{
    if (!incs.empty()) {
        out << "/* extra headers */\n";

        for (auto &e : incs) {
            out << "#include " << e << '\n';
        }
        out << '\n';
    }
}

/* open file for writing */
bool open_ofstream(cio::ofstream &ofs, const std::string &ofile, bool force)
{
    /* check if file already exists by opening it for reading */
    if (!force && ofile != "-") {
        ofs.open(ofile);

        if (ofs.is_open()) {
            std::cerr << "error: file already exists: " << ofile << std::endl;
            return false;
        }

        ofs.close();
    }

    /* open file for writing and truncate it */
    ofs.open(ofile, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!ofs.is_open()) {
        std::cerr << "error: failed to open file for writing: " << ofile << std::endl;
        return false;
    }

    return true;
}

/* create template data (concatenate) */
void create_template_data(
    std::string &header_data,
    std::string &body_data,
    output::format format,
    bool separate)
{
    switch (format)
    {
    default:
        /* FALLTHROUGH */

    case output::c:
        {
            header_data += common_header_data;
            header_data += c_header_data;

            if (separate) {
                body_data = c_body_data;
            } else {
                header_data += c_body_data;
            }
        }
        break;

    case output::cxx:
        {
            header_data += common_header_data;
            header_data += cxx_header_data;

            if (separate) {
                body_data = cxx_body_data;
            } else {
                header_data += cxx_body_data;
            }
        }
        break;

    case output::minimal:
        header_data = min_c_header_data;
        break;

    case output::minimal_cxx:
        header_data = min_cxx_header_data;
        break;
    }
}

} /* end anonymous namespace */


/* read input and tokenize */
bool gendlopen::tokenize_input()
{
    std::string line;
    cio::ifstream ifs;

    /* open file for reading */
    if (!ifs.open(m_ifile)) {
        std::cerr << "error: failed to open file for reading: " << m_ifile << std::endl;
        return false;
    }

    /* check first line */
    if (!ifs.peek_line(line)) {
        std::cerr << "error: failed to read first line from file: " << m_ifile << std::endl;
        return false;
    }

    /* sort vectors and remove duplicates */
    auto sort_vstring = [] (vstring_t &vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };

    sort_vstring(m_prefix);
    sort_vstring(m_symbols);

    utils::strip_ansi_colors(line);

    /* Clang AST */
    if (line.starts_with("TranslationUnitDecl 0x")) {
        /* flags/settings that exclude each other */
        if (m_ast_all_symbols && (!m_symbols.empty() || !m_prefix.empty())) {
            std::cerr << "error: cannot combine `--ast-all-symbols' with `--symbol' or `--prefix'" << std::endl;
            return false;
        }

        /* no symbols provided */
        if (!m_ast_all_symbols && m_symbols.empty() && m_prefix.empty()) {
            std::cerr << "error: Clang AST: no symbols provided to look for; use `--symbol' and/or `--prefix'" << std::endl;
            std::cerr << "You can also pass `--ast-all-symbols' if you REALLY want to use all symbols." << std::endl;
            return false;
        }

        return clang_ast(ifs);
    }

    /* regular tokenizer */
    return tokenize(ifs);
}

/* read and parse custom template */
int gendlopen::parse_custom_template(const std::string &ofile)
{
    cio::ofstream out;
    std::string data;
    char c;

    /* input file */
    std::ifstream ifs(m_custom_template, std::ios::in | std::ios::binary);

    if (!ifs.is_open()) {
        std::cerr << "error: failed to open file for reading: " << m_custom_template << std::endl;
        return 1;
    }

    /* read data */
    while (ifs.get(c) && ifs.good()) {
        data.push_back(c);
    }

    ifs.close();

    /* output file */
    if (!open_ofstream(out, ofile, m_force)) {
        return 1;
    }

    out << parse(data);

    return 0;
}

/* generate output */
int gendlopen::generate(const std::string ifile, const std::string ofile, const std::string name)
{
    std::filesystem::path ofhdr, ofbody;
    std::string header_data, body_data, header_name;
    cio::ofstream out, out_body;

    /* set member variables */
    m_ifile = ifile;
    m_name_upper = utils::convert_to_upper(name);
    m_name_lower = utils::convert_to_lower(name);

    /* tokenize */
    if (!tokenize_input()) {
        return 1;
    }

    /* read and parse custom template (`--format' will be ignored) */
    if (!m_custom_template.empty()) {
        return parse_custom_template(ofile);
    }

    /* output filename */
    ofbody = ofhdr = ofile;

    const bool use_stdout = (ofile == "-");

    if (use_stdout || !separate_is_supported()) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
    if (m_separate) {
        if (output_is_c()) {
            ofhdr.replace_extension(".h");
            ofbody.replace_extension(".c");
        } else {
            ofhdr.replace_extension(".hpp");
            ofbody.replace_extension(".cpp");
        }
    }

    /* create header filename and header guard */
    if (use_stdout) {
        header_name = name;
        header_name += output_is_c() ? ".h" : ".hpp";
    } else {
        header_name = ofhdr.filename().string();
    }

    std::string header_guard = utils::convert_to_upper(header_name);


    /************** header begin ***************/

    if (!open_ofstream(out, ofhdr.string(), m_force)) {
        return 1;
    }

    std::string note = create_note(*m_argc, *m_argv);

    out << note;
    out << "#ifndef _" << header_guard << "_\n";
    out << "#define _" << header_guard << "_\n\n";

    /* insert filename macros BEFORE defines and headers */
    out << filename_macros_data << '\n';

    print_extra_defines(out, m_defines);
    print_default_libname(out, m_name_upper, m_deflib_a, m_deflib_w);
    print_includes(out, m_includes);

    if (output_is_c()) {
        out << extern_c_begin;
    }

    create_template_data(header_data, body_data, m_format, m_separate);
    out << parse(header_data);

    if (output_is_c()) {
        out << extern_c_end;
    }

    out << "\n#endif //_" << header_guard << "_\n";

    out.close();

    if (!m_separate) {
        return 0;
    }

    /************** body data ***************/

    if (!open_ofstream(out_body, ofbody.string(), m_force)) {
        return 1;
    }

    out_body << note;
    out_body << "#include \"" << header_name << "\"\n\n";
    out_body << parse(body_data);

    return 0;
}

