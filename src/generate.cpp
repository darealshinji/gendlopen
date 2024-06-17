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
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>

#include "template.h"
#include "gendlopen.hpp"

namespace fs = std::filesystem;


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

/**
 * convert from string to wstring;
 * this is required because on MinGW std::filesystem will throw an exception
 * if the string contains non-ASCII characters (this doesn't happend with MSVC)
 */
#ifdef __MINGW32__

wchar_t *char_to_wchar(const char *str)
{
    size_t len, n;
    wchar_t *buf;

    if (!str || ::mbstowcs_s(&len, NULL, 0, str, 0) != 0 || len == 0) {
        return nullptr;
    }

    buf = new wchar_t[(len + 1) * sizeof(wchar_t)];
    if (!buf) return nullptr;

    if (::mbstowcs_s(&n, buf, len+1, str, len) != 0 || n == 0) {
        delete[] buf;
        return nullptr;
    }

    buf[len] = L'\0';
    return buf;
}

std::wstring convert_filename(const std::string &str)
{
    wchar_t *buf = char_to_wchar(str.c_str());

    if (!buf) {
        std::cerr << "error: failed to convert string to wide characters: "
            << str << std::endl;
        std::abort();
    }

    std::wstring ws = buf;
    delete[] buf;

    return ws;
}

#define CONV_STR(x)  convert_filename(x)

#else

#define CONV_STR(x)  x

#endif // __MINGW32__


/* get common symbol prefix */
std::string get_common_prefix(const vproto_t &proto, const vproto_t &obj)
{
    std::string pfx = "";
    vstring_t v;

    if ((proto.size() + obj.size()) < 2) {
        return "";
    }

    /* copy symbols into vector list */
    for (const auto &e : proto) {
        v.push_back(e.symbol);
    }

    for (const auto &e : obj) {
        v.push_back(e.symbol);
    }

    /* get shortest symbol length */
    size_t len = v.at(0).size();
    auto it = v.begin() + 1;

    for ( ; it != v.end(); it++) {
        len = std::min(len, (*it).size());
    }

    /* compare symbol names */
    for (size_t i = 0; i < len; i++) {
        const char c = v.at(0)[i];

        for (it = v.begin() + 1; it != v.end(); it++) {
            if ((*it)[i] != c) {
                return pfx;
            }
        }

        pfx.push_back(c);
    }

    return pfx;
}

/* create a note to put at the beginning of the output */
std::string create_note(int &argc, char **&argv)
{
    std::string line = "//";
    std::stringstream out;

    out << "// Do not edit this file!\n";
    out << "// It was created by gendlopen ";

    /* print date */
    struct tm tm = {};
    time_t t = std::time(nullptr);
    bool tm_ok;

#ifdef _WIN32
    tm_ok = (::localtime_s(&tm, &t) == 0);
#else
    tm_ok = (::localtime_r(&t, &tm) != nullptr);
#endif

    if (tm_ok) {
        out << "on " << std::put_time(&tm, "%F %T %z") << '\n';
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


/* open file for writing */
void gendlopen::open_ofstream(cio::ofstream &ofs, const fs::path &opath, bool force)
{
    /* delete file to prevent writing data into symlink target */
    if (force) {
        fs::remove(opath);
    }

    /* check symlink and not its target */
    if (fs::exists(fs::symlink_status(opath))) {
        std::string msg = "file already exists: ";
        msg += opath.string();
        throw error(msg);
    }

    /* open file for writing */
    if (!ofs.open(opath)) {
        std::string msg = "failed to open file for writing: ";
        msg += opath.string();
        throw error(msg);
    }
}

/* read input and tokenize */
void gendlopen::tokenize_input()
{
    std::string peek;
    cio::ifstream ifs;

    /* open file for reading */
    if (!ifs.open(m_ifile)) {
        std::string msg = "failed to open file for reading: ";
        msg += m_ifile;
        throw error(msg);
    }

    /* check first line */
    if (!ifs.peek_line(peek)) {
        std::string msg = "failed to read first line from file: ";
        msg += m_ifile;
        throw error(msg);
    }

    /* sort vectors and remove duplicates */
    auto sort_vstring = [] (vstring_t &vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };

    sort_vstring(m_prefix);
    sort_vstring(m_symbols);

    /* skip UTF-8 Byte Order Mark */
    if (peek.starts_with("\xEF\xBB\xBF")) {
        peek.erase(0, 3);
        ifs.ignore(3);
    }

    utils::strip_ansi_colors(peek);

    /* Clang AST */
    if (peek.starts_with("TranslationUnitDecl 0x")) {
        /* flags/settings that exclude each other */
        if (m_ast_all_symbols && (!m_symbols.empty() || !m_prefix.empty())) {
            throw error("cannot combine `--ast-all-symbols' with `--symbol' or `--prefix'");
        }

        /* no symbols provided */
        if (!m_ast_all_symbols && m_symbols.empty() && m_prefix.empty()) {
            throw error("Clang AST: no symbols provided to look for\n"
                        "use `--symbol', `--prefix' or `--ast-all-symbols'");
        }

        ifs.ignore_line();

        clang_ast(ifs);
    } else {
        /* regular tokenizer */
        tokenize(ifs);
    }

    /* look for a common symbol prefix */
    std::string pfx = get_common_prefix(m_prototypes, m_objects);

    if (pfx.size() > 0) {
        std::stringstream out;
        out << "#define " << m_name_upper << "_COMMON_PFX     \"" << pfx << "\"\n";
        out << "#define " << m_name_upper << "_COMMON_PFX_LEN " << pfx.size() << '\n';
        add_def(out.str());
    }
}

/* read and parse custom template */
void gendlopen::parse_custom_template(const std::string &ofile)
{
    cio::ifstream ifs;
    cio::ofstream out;
    std::string data;
    char c;

    /* open file for reading */
    if (!ifs.open(m_custom_template)) {
        std::string msg = "failed to open file for reading: ";
        msg += m_custom_template;
        throw error(msg);
    }

    /* read data */
    while (ifs.get(c) && ifs.good()) {
        data.push_back(c);
    }

    ifs.close();

    if (ofile != "-") {
        /* create output file */
        open_ofstream(out, ofile, m_force);
    }

    out << parse(data);
}

/* generate output */
void gendlopen::generate(const std::string ifile, const std::string ofile, const std::string name)
{
    fs::path ofhdr, ofbody;
    std::string header_data, body_data, header_name;
    cio::ofstream out, out_body;

    /* set member variables first */
    m_ifile = ifile;
    m_name_upper = utils::convert_to_upper(name);
    m_name_lower = utils::convert_to_lower(name);

    /* tokenize */
    tokenize_input();

    /* read and parse custom template (`--format' will be ignored) */
    if (!m_custom_template.empty()) {
        parse_custom_template(ofile);
        return;
    }

    /* output filename */
    const bool use_stdout = (ofile == "-");

    if (!use_stdout) {
        ofbody = ofhdr = CONV_STR(ofile);
    }

    /* DISABLE separate files on stdout or minimal output */
    if (use_stdout || m_format == output::minimal || m_format == output::minimal_cxx) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
    const bool output_is_c = (m_format != output::cxx && m_format != output::minimal_cxx);

    if (m_separate) {
        if (output_is_c) {
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
        header_name += output_is_c ? ".h" : ".hpp";
    } else {
        header_name = ofhdr.filename().string();
    }

    std::string header_guard = utils::convert_to_upper(header_name);


    /************** header begin ***************/

    if (!use_stdout) {
        open_ofstream(out, ofhdr, m_force);
    }

    std::string note = create_note(m_argc, m_argv);

    out << note;
    out << "#ifndef _" << header_guard << "_\n";
    out << "#define _" << header_guard << "_\n\n";

    if (output_is_c) {
        out << extern_c_begin;
    }

    /* insert filename macros BEFORE defines and headers */
    out << filename_macros_data << '\n';

    print_extra_defines(out, m_defines);
    print_default_libname(out, m_name_upper, m_deflib_a, m_deflib_w);
    print_includes(out, m_includes);

    create_template_data(header_data, body_data, m_format, m_separate);
    out << parse(header_data);

    if (output_is_c) {
        out << extern_c_end;
    }

    out << "\n#endif //_" << header_guard << "_\n";

    out.close();

    /************** body data ***************/

    if (m_separate) {
        if (!use_stdout) {
            open_ofstream(out_body, ofbody, m_force);
        }

        out_body << note;
        out_body << "#include \"" << header_name << "\"\n\n";
        out_body << parse(body_data);
    }
}

