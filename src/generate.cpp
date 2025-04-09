/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

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

/**
 * Generate the output data (STDOUT or save to file).
 */

#ifdef __MINGW32__
# include <stdlib.h>
# include <wchar.h>
#endif
#include <stdio.h>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifndef __cpp_lib_filesystem
# include <sys/types.h>
# include <sys/stat.h>
# ifndef _WIN32
#  include <unistd.h>
# endif
#endif
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "open_file.hpp"
#include "types.hpp"
#include "utils.hpp"



namespace /* anonymous */
{

#ifdef __cpp_lib_filesystem

namespace fs = std::filesystem;

/* use features from std::filesystem */

std::string get_basename(const fs::path &p) {
    return p.filename().string();
}

void replace_extension(fs::path &p, const char *ext) {
    p.replace_extension(ext);
}

void remove_file(const fs::path &p) {
    fs::remove(p);
}

bool exists_lstat(const fs::path &p) {
    return fs::exists(fs::symlink_status(p));
}

#else

/* fall back to using our own implementations */

std::string get_basename(const std::string &str)
{
#ifdef _WIN32
    if (utils::ends_with(str, '\\') || utils::ends_with(str, '/')) {
        return "";
    }

    auto pos = str.find_last_of("\\/");
#else
    if (utils::ends_with(str, '/')) {
        return "";
    }

    auto pos = str.rfind('/');
#endif

    return (pos == std::string::npos) ? str : str.substr(pos+1);
}

void replace_extension(std::string &path, const char *ext)
{
    std::string dirname, basename, dot;

    /* first split path into dirname and basename */
#ifdef _WIN32
    auto pos = path.find_last_of("\\/");
#else
    auto pos = path.rfind('/');
#endif

    if (pos == std::string::npos) {
        basename = path;
    } else {
        basename = path.substr(pos+1);
        dirname = path.substr(0, pos+1);
    }

    /* remove leading dot temporarily from basename */
    if (utils::starts_with(basename, '.')) {
        dot = ".";
        basename.erase(0, 1);
    }

    /* erase old extension */
    pos = basename.rfind('.');

    if (pos != std::string::npos) {
        basename.erase(pos);
    }

    path = dirname + dot + basename + ext;
}

void remove_file(const std::string &path)
{
#ifdef _WIN32
    _unlink(path.c_str());
#else
    unlink(path.c_str());
#endif
}

bool exists_lstat(const std::string &path)
{
#ifdef _WIN32
    /* no lstat/_lstat available */
    struct _stat st;
    return (_stat(path.c_str(), &st) == 0);
#else
    struct stat st;
    return (lstat(path.c_str(), &st) == 0);
#endif
}

#endif // __cpp_lib_filesystem


#ifdef __MINGW32__

/**
 * convert from string to wstring;
 * this is required because on MinGW std::filesystem will throw an exception
 * if the string contains non-ASCII characters (this doesn't happend with MSVC)
 */

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
        std::cerr << "error: mbstowcs_s(): failed to convert string to wide characters: "
            << str << std::endl;
        std::abort();
    }

    std::wstring ws = buf;
    delete[] buf;

    return ws;
}

#else

/* dummy */
template<typename T>
T convert_filename(const T &str) {
    return str;
}

#endif // __MINGW32__


/* read input lines */
bool get_lines(FILE *fp, std::string &line, template_t &entry)
{
    bool loop = true;
    int c = EOF;

    line.clear();
    entry.maybe_keyword = 0;
    entry.line_count = 1;

    while (loop)
    {
        c = fgetc(fp);

        switch (c)
        {
        case '\n':
            /* concatenate lines ending on '@' */
            if (utils::ends_with(line, '@')) {
                line.back() = '\n';
                entry.line_count++;
                continue;
            }
            loop = false;
            break;

        case EOF:
            if (utils::ends_with(line, '@')) {
                line.pop_back();
            }
            loop = false;
            break;

        case '%':
            entry.maybe_keyword = 1;
            [[fallthrough]];

        default:
            line.push_back(static_cast<char>(c));
            continue;
        }
    }

    entry.data = line.c_str();

    return (c == EOF);
}


/* print note */
int save_note(cio::ofstream &out, bool print_date)
{
    std::stringstream strm;

    strm << "// Do not edit this file!\n"
        "// It was automatically generated by gendlopen";

    if (print_date) {
        struct tm tm = {};
        time_t t = std::time(nullptr);
        bool tm_ok;

#ifdef _WIN32
        tm_ok = (::localtime_s(&tm, &t) == 0);
#else
        tm_ok = (::localtime_r(&t, &tm) != nullptr);
#endif

        if (tm_ok) {
            strm << " on " << std::put_time(&tm, "%F %T %z");
        }
    }

    strm << ".\n"
        "// All changes made will be lost.\n"
        "\n";

    out << strm.str();

    return utils::count_linefeed(strm.str());
}

/* extra defines */
int save_extra_defines(cio::ofstream &out, const std::string &defs)
{
    if (defs.empty()) {
        return 0;
    }

    std::string str = "/* extra defines */\n";
    str += defs + '\n';
    out << str;

    return utils::count_linefeed(str);
}

/* typedefs for function pointers */
int save_typedefs(cio::ofstream &out, const vstring_t &tdefs)
{
    if (tdefs.empty()) {
        return 0;
    }

    std::string str = "/* typedefs */\n";

    for (auto &e : tdefs) {
        str += "typedef " + e + ";\n";
    }

    str += '\n';
    out << str;

    return utils::count_linefeed(str);
}

/* extra includes */
int save_includes(cio::ofstream &out, const vstring_t &includes, bool is_cxx)
{
    std::string str = "/* extra headers */\n";

    if (is_cxx) {
        str += "#include <cstddef>\n"  /* size_t, NULL, ... */
               "#include <cstring>\n"; /* strncmp() */
    } else {
        str += "#include <stddef.h>\n"
               "#include <string.h>\n";
    }

    for (auto &e : includes) {
        str += "#include " + e + '\n';
    }

    str += '\n';
    out << str;

    return utils::count_linefeed(str);
}

int save_header_guard_begin(cio::ofstream &out, const std::string &header_guard, bool is_cxx)
{
    std::string str;

    if (header_guard.empty()) {
        str = "\n"
            "#pragma once\n\n";
    } else {
        str = "\n"
            "#ifndef " + header_guard + "\n"
            "#define " + header_guard + "\n\n";
    }

    if (!is_cxx) {
        /* extern C begin */
        str += "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n";
    }

    out << str;

    return utils::count_linefeed(str);
}

/* don't need to count lines anymore */
void save_header_guard_end(cio::ofstream &out, const std::string &header_guard, bool is_cxx)
{
    if (!is_cxx) {
        /* extern C end */
        out << "#ifdef __cplusplus\n"
            "} /* extern \"C\" */\n"
            "#endif\n";
    }

    if (!header_guard.empty()) {
        out << "\n#endif //" << header_guard << "_\n";
    }
}

/* print all found symbols to stdout */
void print_symbols_to_stdout(const vproto_t &objs, const vproto_t &funcs, const vstring_t &tdefs)
{
    cio::ofstream out; /* defaults to STDOUT */
    save_typedefs(out, tdefs);

    std::cout << "/* prototypes */\n";

    auto print_type = [] (const std::string &s)
    {
        if (utils::ends_with(s, '*')) {
            std::cout << s;
        } else {
            std::cout << s << ' ';
        }
    };

    for (const auto &e : objs) {
        print_type(e.type);
        std::cout << e.symbol << ";\n";
    }

    for (const auto &e : funcs) {
        print_type(e.type);
        std::cout << e.symbol << '(' << e.args << ");\n";
    }

    std::cout << "\n/***  " << (objs.size() + funcs.size()) << " matches  ***/" << std::endl;
}

/**
 * Look for a common symbol prefix.
 * Many APIs share a common prefix among their symbols.
 * If you want to load a specific symbol we can use this
 * later for a faster lookup.
 */
size_t common_prefix_length(vstring_t &vec)
{
    /* need at least 2 symbols */
    if (vec.size() < 2) {
        return 0;
    }

    size_t shortest_sym_len = vec.at(0).size();

    /* get shortest symbol length */
    for (const auto &e : vec) {
        /* prevent `min()' macro expansion from Windows headers */
        shortest_sym_len = std::min<size_t>(shortest_sym_len, e.size());
    }

    if (shortest_sym_len == 0) {
        return 0;
    }

    size_t pfxlen = 0;
    const char *ptr = vec.front().c_str();

    for (size_t i = 0; i < shortest_sym_len; i++, ptr++) {
        for (const auto &e : vec) {
            if (e.empty()) {
                return 0;
            }

            if (e.at(i) != *ptr) {
                /* common prefix found (can be empty) */
                return pfxlen;
            }
        }

        pfxlen++;
    }

    /* shortest symbol name is prefix, i.e. if a symbol `foo'
     * and `foobar' exist the prefix is `foo' */
    return pfxlen;
}

/* create a macro that will do a slightly optimized lookup of a
 * given symbol name */
int save_symbol_name_goto(cio::ofstream &out, const std::string &pfx_upper,
    const vproto_t &v_prototypes, const vproto_t &v_objects)
{
    vstring_t symbols, temp;
    std::vector<vstring_t> lists;
    std::string str;

    /* copy symbol names */
    for (const auto &e : v_prototypes) {
        symbols.push_back(e.symbol);
    }

    for (const auto &e : v_objects) {
        symbols.push_back(e.symbol);
    }

    if (symbols.empty()) {
        /* should not happen */
        return 0;
    }

    /* GDO_CHECK_SYMBOL_NAME */
    str = "/* symbol name check */\n"
          "#define " + pfx_upper + "_CHECK_SYMBOL_NAME(SYM) \\\n";

    /* only 1 symbol */
    if (symbols.size() == 1) {
        str += "  if (SYM != NULL && strcmp(SYM, \"" + symbols.at(0) + "\") == 0) { \\\n"
               "    goto " + pfx_upper + "_JUMP_" + symbols.at(0) + "; \\\n"
               "  }\n\n";

        out << str;

        return utils::count_linefeed(str);
    }

    /* multiple symbols */

    const size_t pfxlen = common_prefix_length(symbols); /* can be 0 */
    const auto pfx = symbols.at(0).substr(0, pfxlen); /* can be empty string */
    const auto str_pfxlen = std::to_string(pfxlen);
    const auto off = std::to_string(pfxlen + 1);

    /* prefix check */
    if (pfxlen == 0) {
        str += "  if (SYM != NULL) \\\n";
    } else if (pfxlen == 1) {
        str += "  if (SYM != NULL && *SYM == '" + pfx + "') \\\n";
    } else {
        str += "  if (SYM != NULL && strncmp(SYM, \"" + pfx + "\", " + str_pfxlen + ") == 0) \\\n";
    }

    /* copy symbol names into alphabetically sorted lists */

    std::sort(symbols.begin(), symbols.end());

    for (const auto &e : symbols) {
        if (e.empty()) {
            /* unlikely */
            continue;
        }

        if (!temp.empty() && temp.front().at(pfxlen) != e.at(pfxlen)) {
            lists.push_back(temp);
            temp.clear();
        }

        temp.push_back(e);
    }

    if (!temp.empty()) {
        lists.push_back(temp);
    }

    if (lists.empty()) {
        /* unlikely */
        return 0;
    }

    /* switch begin */
    str += "  { \\\n"
           "    switch (*(SYM+" + str_pfxlen + ")) \\\n"
           "    { \\\n";

    /* switch entries */
    for (const auto &v : lists) {
        const char c = utils::str_at(v.front(), pfxlen);

        if (c == 0) {
            str += "    case 0: /* same as common symbol prefix */ \\\n"
                   "      goto " + pfx_upper + "_JUMP_" + pfx + "; \\\n";
        } else {
            str += "    case '"; str+=c; str+="': \\\n";

            for (const auto &symbol : v) {
                const char *short_name = symbol.c_str() + pfxlen + 1;

                str += "      if (strcmp(SYM+" + off + ", \"" + short_name + "\") == 0) /*" + symbol + "*/ \\\n"
                       "        { goto " + pfx_upper + "_JUMP_" + symbol + "; } \\\n";
            }

            str += "      break; \\\n";
        }
    }

    /* switch end */
    str += "    default: \\\n"
           "      break; \\\n"
           "    } \\\n"
           "  }\n\n";

    out << str;

    return utils::count_linefeed(str);
}

} /* end anonymous namespace */


/* open output file for writing */
void gendlopen::open_ofstream(const fs_path_t &opath, cio::ofstream &ofs)
{
    std::string opath_str;

#ifdef __cpp_lib_filesystem
    opath_str = opath.string();
#else
    opath_str = opath;
#endif

    if (opath.empty() || opath_str == "-") {
        /* default to STDOUT */
        ofs.close();
        return;
    }

    /* delete file to prevent writing data into symlink target */
    if (m_force) {
        remove_file(opath);
    }

    /* check symlink and not its target */
    if (exists_lstat(opath)) {
        throw error("file already exists: " + opath_str);
    }

    /* open file for writing */
    if (!ofs.open(opath)) {
        throw error("failed to open file for writing: " + opath_str);
    }
}

/* read and process custom template */
void gendlopen::read_custom_template()
{
    cio::ofstream ofs;
    std::string buf;
    template_t entry;
    bool param_skip_code = false;
    bool eof = false;
    int templ_lineno = 1; /* input template line count */

    /* open file for reading */
    open_file file(m_custom_template);

    if (!file.is_open()) {
        throw error("failed to open file for reading: " + m_custom_template);
    }

    /* create output file */
    open_ofstream(m_output, ofs);
    FILE *fp = file.file_pointer();

    /* write initial #line directive */
    if (m_line_directive) {
        if (fp == stdin) {
            ofs << "#line 1 \"<STDIN>\"\n";
        } else {
            ofs << "#line 1 \"" << m_custom_template << "\"\n";
        }
    }

    /* parse lines */
    while (!eof) {
        eof = get_lines(fp, buf, entry);
        substitute_line(entry, templ_lineno, param_skip_code, ofs);
        templ_lineno += entry.line_count;
    }
}

/* generate output */
void gendlopen::generate()
{
    cio::ofstream ofs;
    fs_path_t ofhdr, ofbody;
    std::string header_name, header_guard;
    vtemplate_t header_data, body_data;
    int lines = 0;
    bool is_cxx = false;
    bool is_minimal = false;

    /************* lambda functions *************/

    auto print_lineno = [&, this] () {
        if (m_line_directive) {
            lines++;
            ofs << "#line " << (lines + 1) << " \"" << header_name << "\"\n";
        }
    };

    auto HEADER_NOTE_AND_LICENSE = [&, this] () {
        lines += save_note(ofs, m_print_date);
        lines += save_license_data(ofs);
    };

    auto HEADER_GUARD_BEGIN = [&, this] () {
        if (!m_pragma_once) {
            header_guard = '_' + m_pfx_upper + '_' + utils::convert_to_upper(header_name) + '_';
        }
        lines += save_header_guard_begin(ofs, header_guard, is_cxx);
    };

    auto HEADER_FILENAME_MACROS = [&] () {
        lines += save_filename_macros_data(ofs);
    };

    auto HEADER_GENERATED_DATA = [&, this] () {
        /* print extra data after filename macros as includes or defines
         * might make use of it */
        if (m_line_directive) {
            print_lineno();
            ofs << '\n'; /* extra padding */
            lines++;
        }

        lines += save_extra_defines(ofs, m_defines);
        lines += save_includes(ofs, m_includes, is_cxx);
        lines += save_typedefs(ofs, m_typedefs);

        /* minimal/plugin headers don't need this */
        if (!is_minimal && m_format != output::plugin) {
            lines += save_symbol_name_goto(ofs, m_pfx_upper, m_prototypes, m_objects);
        }
    };

    auto HEADER_TEMPLATE_DATA = [&] () {
        lines += substitute(header_data, ofs);
    };

    auto HEADER_GUARD_END = [&] () {
        print_lineno();
        save_header_guard_end(ofs, header_guard, is_cxx);
    };

    auto BODY_NOTE_AND_LICENSE = [&, this] () {
        save_note(ofs, m_print_date);
        save_license_data(ofs);
    };

    auto BODY_INCLUDE_HEADER = [&, this] () {
        ofs << '\n';
        ofs << "#define " << m_pfx_upper << "_INCLUDED_IN_BODY\n";
        ofs << "#include \"" << header_name << "\"\n\n";
    };

    auto BODY_TEMPLATE_DATA = [&] () {
        substitute(body_data, ofs);
    };

    /********************************************/


    /* tokenize and parse strings from input */
    tokenize();

    /* print symbols and exit */
    if (m_print_symbols) {
        print_symbols_to_stdout(m_objects, m_prototypes, m_typedefs);
        return;
    }

    /* use custom template (`-format' will be ignored) */
    if (!m_custom_template.empty()) {
        read_custom_template();
        return;
    }

    /* output filename */

    const bool use_stdout = (m_output == "-");

    if (!use_stdout) {
        ofbody = ofhdr = convert_filename(m_output);
    }

    switch (m_format)
    {
    case output::c:
    case output::plugin:
        break;

    case output::cxx:
        is_cxx = true;
        break;

    case output::minimal:
        m_separate = false;
        is_minimal = true;
        break;

    case output::minimal_cxx:
        is_cxx = true;
        m_separate = false;
        is_minimal = true;
        break;

    [[unlikely]] case output::error:
        throw error(std::string(__func__) + ": m_format == output::error");
    }

    /* disable separate files on stdout */
    if (use_stdout) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
    if (m_separate) {
        if (is_cxx) {
            replace_extension(ofhdr, ".hpp");
            replace_extension(ofbody, ".cpp");
        } else {
            replace_extension(ofhdr, ".h");
            replace_extension(ofbody, ".c");
        }
    }

    /* create header filename */
    if (use_stdout) {
        header_name = m_pfx;
        header_name += is_cxx ? ".hpp" : ".h";
    } else {
        header_name = get_basename(ofhdr);
    }

    /* define GDO_SEPARATE if saving into separate files */
    if (m_separate) {
        m_defines += "#define " + m_pfx_upper + "_SEPARATE\n";
    }

    /* default library name */
    if (!m_deflib_a.empty() && !m_deflib_w.empty()) {
        m_defines += "#define " + m_pfx_upper + "_HARDCODED_DEFAULT_LIBA " + m_deflib_a + '\n';
        m_defines += "#define " + m_pfx_upper + "_HARDCODED_DEFAULT_LIBW " + m_deflib_w + '\n';
    }

    /* save pointers to template lines in header_data and body_data */
    create_template_data_lists(header_data, body_data);


    /*************** header data ***************/
    open_ofstream(ofhdr, ofs);

    HEADER_NOTE_AND_LICENSE();
    HEADER_GUARD_BEGIN();

    HEADER_FILENAME_MACROS();
    HEADER_GENERATED_DATA();

    HEADER_TEMPLATE_DATA();

    HEADER_GUARD_END();

    ofs.close();


    /**************** body data ****************/
    if (!body_data.empty()) {
        open_ofstream(ofbody, ofs);

        BODY_NOTE_AND_LICENSE();
        BODY_INCLUDE_HEADER();

        BODY_TEMPLATE_DATA();
    }
}

