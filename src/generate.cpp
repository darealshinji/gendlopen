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

#include "gendlopen.hpp"


namespace fs = std::filesystem;


namespace /* anonymous */
{
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

std::wstring convert_filename(const char *str)
{
    wchar_t *buf = char_to_wchar(str);

    if (!buf) {
        std::cerr << "error: failed to convert string to wide characters: "
            << str << std::endl;
        std::abort();
    }

    std::wstring ws = buf;
    delete[] buf;

    return ws;
}

#endif // __MINGW32__


/* get common symbol prefix */
std::string get_common_prefix(const vproto_t &proto, const vproto_t &obj)
{
    std::string pfx;
    std::vector<const char *> v;

    const size_t vlen = proto.size() + obj.size();

    if (vlen < 2) {
        return "";
    }

    v.reserve(vlen);

    /* add symbols to vector list */
    for (const auto &e : proto) {
        v.push_back(e.symbol.c_str());
    }

    for (const auto &e : obj) {
        v.push_back(e.symbol.c_str());
    }

    /* get shortest symbol length */
    size_t len = strlen(v.at(0));

    for (auto it = v.begin() + 1; it != v.end(); it++) {
        /* prevent `min()' macro expansion
         * https://stackoverflow.com/a/30924806/5687704 */
        len = std::min<size_t>(len, strlen(*it));
    }

    /* compare symbol names */
    for (size_t i = 0; i < len; i++) {
        const char c = v.at(0)[i];

        for (auto it = v.begin() + 1; it != v.end(); it++) {
            if ((*it)[i] != c) {
                return pfx;
            }
        }

        pfx.push_back(c);
    }

    return pfx;
}

/* create a note to put at the beginning of the output */
std::string create_note(vstring_t &args)
{
    std::string line = "//";
    std::stringstream out;

    out << "// Do not edit this file!\n"
        "// It was created by gendlopen ";

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
        out << "on " << std::put_time(&tm, "%F %T %z");
        out << "\n// ";
    }
    out << "using the following flags:\n\n";

    /* print used flags */

    for (auto &e : args) {
        /* split long lines */
        if ((line.size() + 1 + e.size()) > 80) {
            out << line << '\n';
            line = "//";
        }
        line += ' ' + e;
    }

    out << line << "\n\n";

    return out.str();
}

/* define default library name */
inline void print_default_libname(
    cio::ofstream &out,
    const std::string &pfx,
    const std::string &lib_a,
    const std::string &lib_w)
{
    if (!lib_a.empty() && !lib_w.empty()) {
        out << "/* default library */\n"
            "#ifndef "  << pfx << "_DEFAULT_LIBA\n"
            "# define " << pfx << "_DEFAULT_LIBA " << lib_a << "\n"
            "#endif\n"
            "#ifndef "  << pfx << "_DEFAULT_LIBW\n"
            "# define " << pfx << "_DEFAULT_LIBW " << lib_w << "\n"
            "#endif\n\n";
    }
}

/* extra defines */
inline void print_extra_defines(cio::ofstream &out, const std::string &defs)
{
    if (!defs.empty()) {
        out << "/* extra defines */\n";
        out << defs << '\n';
    }
}

/* extra includes */
inline void print_includes(cio::ofstream &out, const vstring_t &incs)
{
    if (!incs.empty()) {
        out << "/* extra headers */\n";

        for (auto &e : incs) {
            out << "#include " << e << '\n';
        }
        out << '\n';
    }
}

/* print all found symbols */
void print_symbols_to_stdout(const vproto_t &objects, const vproto_t &functions)
{
    for (const auto &e : objects) {
        std::cout << e.type;

        if (!e.type.ends_with(" *")) {
            std::cout << ' ';
        }
        std::cout << e.symbol << ";\n";
    }

    for (const auto &e : functions) {
        std::cout << e.type;

        if (!e.type.ends_with(" *")) {
            std::cout << ' ';
        }
        std::cout << e.symbol << '(' << e.args << ");\n";
    }

    std::cout << "/***  " << (objects.size() + functions.size()) << " matches  ***/" << std::endl;
}

inline bool has_ignore_commands_set(const std::string &line)
{
    std::string token;
    std::istringstream iss(line);

    while (iss >> token) {
        if (token == "-ignore-commands") {
            return true;
        }
    }

    return false;
}

} /* end anonymous namespace */


/* open file for writing */
void gendlopen::open_ofstream(const fs::path &opath, bool force, bool body)
{
    /* delete file to prevent writing data into symlink target */
    if (force) {
        fs::remove(opath);
    }

    /* check symlink and not its target */
    if (fs::exists(fs::symlink_status(opath))) {
        throw error("file already exists: " + opath.string());
    }

    /* open file for writing */
    if ((body && !m_ofs_body.open(opath)) || (!body && !m_ofs.open(opath))) {
        throw error("failed to open file for writing: " + opath.string());
    }
}

/* read input and tokenize */
void gendlopen::tokenize_input()
{
    std::string peek;

    /* open file for reading */
    if (!m_ifs.open(m_ifile)) {
        throw error("failed to open file for reading: " + m_ifile);
    }

    /* check first line */
    if (!m_ifs.peek_line(peek)) {
        throw error("failed to read first line from file: " + m_ifile);
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
        m_ifs.ignore(3);
    }

    /* read extra commands from file */
    if (m_read_extra_cmds && peek.starts_with("//@CMD")) {
        peek.erase(0, 6);
        utils::strip_spaces(peek);

        if (!peek.empty() && !has_ignore_commands_set(peek)) {
            throw cmdline(peek);
        }
    }

    utils::strip_ansi_colors(peek);

    if (peek.starts_with("TranslationUnitDecl 0x")) {
        /* Clang AST */
        if (m_ast_all_symbols) {
            /* flags/settings that exclude each other */
            if (!m_symbols.empty() || !m_prefix.empty()) {
                throw error("cannot combine `-ast-all-symbols' with `-S' or `-P'");
            }
        } else {
            /* no symbols provided */
            if (m_symbols.empty() && m_prefix.empty()) {
                throw error("Clang AST: no symbols provided to look for\n"
                            "use `-S', `-P' or `-ast-all-symbols'");
            }
        }

        m_ifs.ignore_line();
        clang_ast();
    } else {
        /* regular tokenizer */
        tokenize();
    }

    /* cosmetics */
    auto format_args = [] (vproto_t &vec)
    {
        for (auto &p : vec) {
            utils::replace("* ", "*", p.args);
            utils::replace(" ,", ",", p.args);

            utils::replace("( ", "(", p.args);
            utils::replace(" )", ")", p.args);
            utils::replace(") (", ")(", p.args);

            utils::replace("[ ", "[", p.args);
            utils::replace("] ", "]", p.args);
            utils::replace(" [", "[", p.args);
            utils::replace(" ]", "]", p.args);
        }
    };

    format_args(m_prototypes);
    format_args(m_objects);
}

/* read and process custom template */
void gendlopen::read_custom_template(const std::string &ofile, bool use_stdout)
{
    cstrList_t data;
    cio::ofstream out;
    std::string buf, line;
    cstrList_t list;
    const char *twolines[2] = { NULL, "" };

    /* open file for reading */
    if (!m_ifs.open(m_custom_template)) {
        throw error("failed to open file for reading: " + m_custom_template);
    }

    if (!use_stdout) {
        /* create output file */
        open_ofstream(ofile, m_force, false);
    }

    while (m_ifs.getline(buf)) {
        /* concat lines ending on '@' */
        if (buf.back() == '@') {
            buf.pop_back();
            line += buf + '\n';
            continue;
        }

        line += buf;

        twolines[0] = line.c_str();
        list.push_back(twolines);
        substitute(list, m_ofs);

        list.clear();
        line.clear();
    }
}

/* generate output */
void gendlopen::generate()
{
    fs::path ofhdr, ofbody;
    std::string header_name;
    cstrList_t header_data, body_data;

    if (m_ifile.empty()) {
        throw error("input file required");
    }

    if (m_ifile == "-" && m_custom_template == "-") {
        throw error("cannot read input file and custom template both from STDIN");
    }

    m_name_upper = utils::convert_to_upper(m_name);
    m_name_lower = utils::convert_to_lower(m_name);

    /* tokenize */
    tokenize_input();

    /* print symbols and exit */
    if (m_print_symbols) {
        print_symbols_to_stdout(m_objects, m_prototypes);
        return;
    }

    /* look for a common symbol prefix */
    m_common_prefix = get_common_prefix(m_prototypes, m_objects);

    const bool use_stdout = (m_ofile == "-");

    /* custom template (`-format' will be ignored) */
    if (!m_custom_template.empty()) {
        read_custom_template(m_ofile, use_stdout);
        return;
    }

    /* output filename */

    if (!use_stdout) {
#ifdef __MINGW32__
        ofbody = ofhdr = convert_filename(m_ofile);
#else
        ofbody = ofhdr = m_ofile;
#endif
    }

    bool output_is_c = true;

    switch (m_format)
    {
    case output::c:
        break;

    case output::cxx:
        output_is_c = false;
        break;

    case output::minimal:
        m_separate = false;
        break;

    case output::minimal_cxx:
        output_is_c = false;
        m_separate = false;
        break;

    [[unlikely]] case output::error:
        throw error("output::format == output::error");
    }

    /* disable separate files on stdout */
    if (use_stdout) {
        m_separate = false;
    }

    /* rename file extensions only if we save into separate files */
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
        header_name = m_name;
        header_name += output_is_c ? ".h" : ".hpp";
    } else {
        header_name = ofhdr.filename().string();
    }

    const std::string header_guard = utils::convert_to_upper(header_name.c_str());


    /************** header begin ***************/

    if (!use_stdout) {
        open_ofstream(ofhdr, m_force, false);
    }

    const std::string note = create_note(m_args);

    m_ofs << note;
    data::save_license_data(m_ofs);
    m_ofs << "#ifndef _" << header_guard << "_\n"
             "#define _" << header_guard << "_\n\n";

    if (output_is_c) {
        m_ofs << "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n";
    }

    /* insert filename macros BEFORE defines and headers */
    data::save_filename_macros_data(m_ofs);
    m_ofs << '\n';

    print_extra_defines(m_ofs, m_defines);
    print_default_libname(m_ofs, m_name_upper, m_deflib_a, m_deflib_w);
    print_includes(m_ofs, m_includes);

    data::concat_templates(header_data, body_data, m_format, m_separate);
    substitute(header_data, m_ofs);

    if (output_is_c) {
        m_ofs << "\n"
            "#ifdef __cplusplus\n"
            "} /* extern \"C\" */\n"
            "#endif\n";
    }

    m_ofs << "\n#endif //_" << header_guard << "_\n";
    m_ofs.close();

    /************** body data ***************/

    if (m_separate) {
        if (!use_stdout) {
            open_ofstream(ofbody, m_force, true);
        }

        m_ofs_body << note;
        data::save_license_data(m_ofs_body);
        m_ofs_body << "#include \"" << header_name << "\"\n\n";
        substitute(body_data, m_ofs_body);
    }
}

