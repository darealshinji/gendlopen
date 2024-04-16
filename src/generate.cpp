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

inline struct tm *localtime_wrapper(const time_t *timep, struct tm *tm)
{
#ifdef _WIN32
    return (::localtime_s(tm, timep) == 0) ? tm : nullptr;
#else
    return ::localtime_r(timep, tm);
#endif
}

/* convert input string to be used as prefixes or header guards */

std::string convert_to_upper(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (utils::range(c, 'a','z')) {
            out += c - 32;
        } else if (utils::range(c, 'A','Z') || utils::range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

std::string convert_to_lower(const std::string &in)
{
    std::string out;

    for (const char &c : in) {
        if (utils::range(c, 'A','Z')) {
            out += c + 32;
        } else if (utils::range(c, 'a','z') || utils::range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

/* create a note to put at the beginning of the output */
std::string create_note(int &argc, char ** &argv)
{
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

/* create "#define" lines */
std::string format_definitions(const std::vector<std::string> &list)
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
        out << "#endif\n";
    }

    return out.str();
}

/* open file for writing */
bool open_fstream(cio::ofstream &ofs, const std::string &ofile, bool force)
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
        return;

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
        return;

    case output::minimal:
        header_data = min_c_header_data;
        return;

    case output::minimal_cxx:
        header_data = min_cxx_header_data;
        return;
    }

    utils::unreachable();
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

    /* Clang AST without escape code */
    if (line.starts_with("TranslationUnitDecl 0x")) {
        std::cerr << "error: Clang AST: `TranslationUnitDecl' line found but ANSI escape code is missing" << std::endl;
        std::cerr << "Try adding `-fansi-escape-codes' to clang's flags." << std::endl;
        return false;
    }

    /* sort vectors and remove duplicates */
    auto sort_vstring = [] (vstring_t &vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };

    sort_vstring(m_prefix);
    sort_vstring(m_symbols);

    /* Clang AST */
    if (line.starts_with(sCFGREEN "TranslationUnitDecl" sC0 sCORANGE " 0x")) {
        if (m_symbols.empty() && m_prefix.empty() && !m_ast_all_symbols) {
            std::cerr << "error: Clang AST: no symbols provided to look for; use `--symbol' and/or `--prefix'" << std::endl;
            std::cerr << "You can also pass `--ast-all-symbols' if you REALLY want to use all symbols." << std::endl;
            return false;
        }

        return clang_ast(ifs);
    }

    /* regular tokenizer */
    return tokenize(ifs);
}

/* generate output */
int gendlopen::generate(const std::string &ofile, const std::string &name)
{
    /* assert if only one of them was set */
    assert((m_default_liba.empty() && m_default_libw.empty()) ||
            (!m_default_liba.empty() && !m_default_libw.empty()));

    /* tokenize */
    if (!tokenize_input()) {
        return 1;
    }

    /* is output C or C++? */
    const bool is_c = (m_format != output::cxx && m_format != output::minimal_cxx);

    /* output filename */
    std::filesystem::path ofhdr(ofile);
    auto ofbody = ofhdr;
    const bool use_stdout = (ofile == "-");

    if (use_stdout || (m_separate && !separate_is_supported(m_format))) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
    if (m_separate) {
        if (is_c) {
            ofhdr.replace_extension(".h");
            ofbody.replace_extension(".c");
        } else {
            ofhdr.replace_extension(".hpp");
            ofbody.replace_extension(".cpp");
        }
    }

    /* create header filename and header guard */
    std::string header_name;

    if (use_stdout) {
        header_name = name;
        header_name += is_c ? ".h" : ".hpp";
    } else {
        header_name = ofhdr.filename().string();
    }

    std::string header_guard = convert_to_upper(header_name);

    /* name used on variables and macros */
    m_name_upper = convert_to_upper(name);
    m_name_lower = convert_to_lower(name);


    /************** header begin ***************/

    /* open file */
    cio::ofstream out;

    if (!open_fstream(out, ofhdr.string(), m_force)) {
        return 1;
    }

    /* notification + license text */
    std::string note = create_note(*m_argc, *m_argv);
    out << note;

    /* header guard begin */
    out << "#ifndef _" << header_guard << "_\n";
    out << "#define _" << header_guard << "_\n\n";

    /* insert filename macros before definitions and headers */
    out << filename_macros_data << '\n';

    /* extra definitions */
    if (!m_definitions.empty()) {
        out << "/* extra definitions */\n";
        out << format_definitions(m_definitions);
        out << '\n';
    }

    /* default library name */
    if (!m_default_liba.empty() && !m_default_libw.empty()) {
        out << "/* default library */\n";
        out << "#ifndef " << m_name_upper << "_DEFAULT_LIBA\n";
        out << "#define " << m_name_upper << "_DEFAULT_LIBA " << m_default_liba << "\n";
        out << "#endif\n";
        out << "#ifndef " << m_name_upper << "_DEFAULT_LIBW\n";
        out << "#define " << m_name_upper << "_DEFAULT_LIBW " << m_default_libw << "\n";
		out << "#endif\n";
    }

    /* extra includes */
    if (!m_includes.empty()) {
        out << "/* extra headers */\n";

        for (auto &e : m_includes) {
            out << "#include " << e << '\n';
        }
        out << '\n';
    }

    /* extern "C" begin */
    if (is_c) {
        out << "#ifdef __cplusplus\n";
        out << "extern \"C\" {\n";
        out << "#endif\n\n";
    }

    /* create template code */
    std::string header_data, body_data;
    create_template_data(header_data, body_data, m_format, m_separate);

    /* parse header template */
    out << parse(header_data);

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

    out.close();

    if (!use_stdout) {
        //std::cout << "saved to file: " << ofhdr << std::endl;
    }

    if (!m_separate) {
        return 0;
    }

    /************** header end ***************/


    /* body data */

    cio::ofstream out_body;

    if (!open_fstream(out_body, ofbody.string(), m_force)) {
        return 1;
    }

    out_body << note;
    out_body << "#include \"" << header_name << "\"\n\n";
    out_body << parse(body_data);

    //std::cout << "saved to file: " << ofbody << std::endl;

    return 0;
}

