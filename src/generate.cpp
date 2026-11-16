/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2026 Carsten Janssen

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

#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "open_file.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;



/* template.h */
namespace templates
{
    extern const char *license;
    extern const char *filename_macros;
}


namespace /* anonymous */
{
    bool compare_chars_at(vproto_t &vec, vproto_t::iterator &it0, size_t pos)
    {
        const char *front = (*it0).symbol.c_str();

        for (auto it = vec.begin(); it != vec.end(); it++) {
            if (it != it0) {
                char c = (*it).symbol.c_str()[pos];

                if (c == 0 || c != front[pos]) {
                    return true;
                }
            }
        }

        return false;
    }


    /**
     * Look for a common symbol prefix.
     * Many APIs share a common prefix among their symbols.
     * If you want to load a specific symbol we can use this
     * later for a faster lookup.
     */
    std::string get_common_prefix(vproto_t &v_prot, vproto_t &v_obj)
    {
        /* need at least 2 symbols */
        if ((v_prot.size() + v_obj.size()) < 2) {
            return {};
        }

        size_t i;
        auto it0 = v_prot.empty() ? v_obj.begin() : v_prot.begin();

        /* compare each letter of every entry */
        for (i = 0; i < (*it0).symbol.size(); i++) {
            if (compare_chars_at(v_prot, it0, i) ||
                compare_chars_at(v_obj, it0, i))
            {
                break;
            }
        }

        return (*it0).symbol.substr(0, i);
    }

} /* end anonymous namespace */


namespace save
{

cio::ofstream ofs;


/* quote library name */
std::string quote_lib(const std::string &lib, bool wide)
{
    if (wide) {
        if (lib.size() >= 3 && utils::front_and_back(lib, "L\"", '"')) {
            /* already quoted */
            return lib;
        } else if (lib.size() >= 2 && utils::front_and_back(lib, '"', '"')) {
            /* prepend 'L' */
            return 'L' + lib;
        }

        return "L\"" + lib + '"';
    }

    if (lib.size() >= 2 && utils::front_and_back(lib, '"', '"')) {
        /* already quoted */
        return lib;
    }

    return '"' + lib + '"';
}


/**
 * format library name
 * foo        ==>  "foo"
 * nq:foo     ==>  foo
 * ext:foo    ==>  "foo" GDO_LIBEXTA
 * api:2:foo  ==>  GDO_LIBNAMEA(foo,2)
 *
 * return macros
 */
std::string format_libname(const std::string &str, const std::string &pfx)
{
    std::string lib_a, lib_w, out;

    switch(str.front())
    {
    case 'N':
    case 'n':
        /* no quotes */
        if (utils::prefixed_and_longer_case(str, "nq:")) {
            lib_a = lib_w = str.substr(3);
            break;
        }
        return {};

    case 'E':
    case 'e':
        /* quotes + file extension macro */
        if (utils::prefixed_and_longer_case(str, "ext:")) {
            auto sub = str.substr(4);
            lib_a = quote_lib(sub, false) + ' ' + pfx + "_LIBEXTA";
            lib_w = quote_lib(sub, true)  + ' ' + pfx + "_LIBEXTW";
            break;
        }
        return {};

    case 'A':
    case 'a':
        /* no quotes, API libname macro */
        if (utils::prefixed_and_longer_case(str, "api:")) {
            const std::regex reg("(.*?):(.*)");
            std::smatch m;
            auto sub = str.substr(4);

            if (std::regex_match(sub, m, reg) && m.size() == 3) {
                /* GDO_LIBNAMEA(xxx,0) */
                lib_a = pfx + "_LIBNAMEA(" + m[2].str() + ',' + m[1].str() + ')';
                lib_w = pfx + "_LIBNAMEW(" + m[2].str() + ',' + m[1].str() + ')';
                break;
            }
        }
        return {};

    default:
        /* quote string */
        lib_a = quote_lib(str, false);
        lib_w = quote_lib(str, true);
        break;
    }

    out = "#define " + pfx + "_HARDCODED_DEFAULT_LIBA  " + lib_a + '\n' +
          "#define " + pfx + "_HARDCODED_DEFAULT_LIBW  " + lib_w + '\n';

    return out;
}


/* print note */
size_t note(bool print_date)
{
    std::stringstream strm;

    strm << "// Do not edit this file!\n"
            "// It was automatically generated by gendlopen";

    if (print_date) {
        struct tm tm = {};
        time_t t = ::time(nullptr);
        bool tm_ok;

#ifdef _WIN32
        tm_ok = (::localtime_s(&tm, &t) == 0);
#else
        tm_ok = (::localtime_r(&t, &tm) != nullptr);
#endif

        if (tm_ok) {
            strm << " on " << std::put_time(&tm, "%a %Y-%m-%d %H:%M:%S %z");
        }
    }

    strm << ".\n"
        "// All changes made will be lost.\n"
        "\n";

    ofs << strm.str();

    return utils::count_linefeed(strm.str());
}


/* extra defines, headers and typedefs */
size_t extra_data(const std::string &defs, const vstring_t &incs, const vstring_t &tdefs)
{
    std::string str;

    if (!defs.empty()) {
        str += "\n/* extra defines */\n";
        str += defs;
    }

    if (!incs.empty()) {
        str += "\n/* extra headers */\n";

        for (auto &e : incs) {
            str += "#include " + e + '\n';
        }
    }

    if (!tdefs.empty()) {
        str += "\n/* typedefs */\n";

        for (const auto &e : tdefs) {
            str += "typedef " + e + ";\n";
        }
    }

    str += "\n\n\n\n";

    ofs << str;

    return utils::count_linefeed(str);
}

size_t header_guard_begin(const std::string &header_name, const std::string &pfx_upper, bool pragma_once)
{
    std::string str;

    if (pragma_once) {
        str = "\n"
            "#pragma once\n\n";
    } else {
        std::string guard = '_' + pfx_upper + '_' + utils::to_upper(header_name) + '_';
        str = "\n"
            "#ifndef " + guard + "\n"
            "#define " + guard + "\n\n";
    }

    ofs << str;

    return utils::count_linefeed(str);
}

/* don't need to count lines anymore */
void header_guard_end(const std::string &header_name, const std::string &pfx_upper)
{
    ofs << "#endif //_" << pfx_upper << '_' << utils::to_upper(header_name) << "_\n";
}

void include_header(const std::string &header_name, const std::string &pfx_upper)
{
    /* define "GDO_INCLUDED_IN_BODY" before including the header */
    ofs << "\n"
        "#define " << pfx_upper << "_INCLUDED_IN_BODY\n"
        "#include \"" << header_name << "\"\n"
        "\n";
}

/* open output file stream for writing */
void open_ofstream(const fs::path &opath, bool force)
{
    if (opath.empty() || opath == "-") {
        /* default to STDOUT */
        ofs.close();
        return;
    }

    /* delete file to prevent writing data into symlink target */
    if (force) {
        fs::remove(opath);
    }

    /* check symlink and not its target */
    if (fs::exists(fs::symlink_status(opath))) {
        throw gendlopen::error("file already exists: " + opath.string());
    }

    /* open file for writing */
    if (!ofs.open(opath)) {
        throw gendlopen::error("failed to open file for writing: " + opath.string());
    }
}

size_t line_directive(const size_t lines, bool line_directive, const std::string &header_name)
{
    if (!line_directive) {
        return 0;
    }

    /* +1 to count the "#line" directive itself and
     * +1 to print the number of the next line */
    save::ofs << "#line " << (lines + 2) << " \"" << header_name << "\"\n";

    return 1;
}

} /* end namespace save */


/* save data, replace prefixes, return line count */
size_t gendlopen::save_data(const char *ptr)
{
    std::string data;
    size_t pos = 0;

    load_template(ptr, data);

    /* skip initial line directive */
    if (!m_line_directive && data.starts_with("#line") &&
        utils::find(data, '\n', pos))
    {
        data.erase(0, pos);
    }

    if (m_pfx_upper != "GDO") {
        data = replace_prefixes(data);
    }

    /* save data, count newlines */
    save::ofs << data;

    return utils::count_linefeed(data);
}


/* generate output */
void gendlopen::generate()
{
    fs::path ofhdr, ofbody;
    std::string header_name, header_guard;
    std::string header_data, body_data;
    bool is_cxx = false;
    size_t lines = 0;

    const bool use_stdout = (m_output == "-");

    if (use_stdout) {
        /* disable separate files on stdout */
        m_separate = false;
    } else {
        /* save output filename */
        ofbody = ofhdr = utils::make_fs_path(m_output);
    }

    /* change settings based on output format */
    if (m_format == output::minimal) {
        m_separate = false;
    } else if (m_format == output::minimal_cxx) {
        m_separate = false;
        is_cxx = true;
    }

    /* rename file extensions only if we save into separate files */
    if (m_separate) {
        ofhdr.replace_extension(is_cxx ? "hpp" : "h");
        ofbody.replace_extension(is_cxx ? "cpp" : "c");
    }

    /* create header filename */
    if (use_stdout) {
        header_name = m_pfx;
        header_name += is_cxx ? ".hpp" : ".h";
    } else {
        header_name = ofhdr.filename().string();
    }

    /* define GDO_SEPARATE if saving into separate files */
    if (m_separate) {
        m_defines += "#define " + m_pfx_upper + "_SEPARATE\n";
    } else {
        m_defines += "#undef " + m_pfx_upper + "_SEPARATE\n";
    }

    /* default library name */
    if (!m_default_lib.empty()) {
        m_defines += save::format_libname(m_default_lib, m_pfx_upper);
    }

    /* define common symbol prefix (can be an empty string) */
    m_common_prefix = get_common_prefix(m_prototypes, m_objects);
    m_defines += "#define " + m_pfx_upper + "_COMMON_PREFIX  \"" + m_common_prefix + "\"\n";

    /* define if a prototype has variable arguments */
    for (const auto &e : m_prototypes) {
        if (e.args.ends_with("...")) {
            m_defines += "#define " + m_pfx_upper + "_HAS_VA_ARGS_" + e.symbol + "  1\n";
        }
    }

    /* create template data */
    create_template_lists(header_data, body_data);


    /* save header data */
    save::open_ofstream(ofhdr, m_force);

    lines += save::note(m_print_date);
    lines += save_data(templates::license);

    lines += save::line_directive(lines, m_line_directive, header_name);
    lines += save::header_guard_begin(header_name, m_pfx_upper, m_pragma_once);

    lines += save_data(templates::filename_macros);
    lines += save::line_directive(lines, m_line_directive, header_name);
    lines += save::extra_data(m_defines, m_includes, m_typedefs);

    lines += substitute(header_data);

    if (!m_separate && !body_data.empty()) {
        /* C body data as part of header */
        save::ofs << "\n\n\n";
        lines += 3;
        lines += substitute(body_data);
    }

    if (!m_pragma_once) {
        save::line_directive(lines, m_line_directive, header_name);
        save::header_guard_end(header_name, m_pfx_upper);
    }


    /* save body data in separate file */
    if (m_separate && !body_data.empty()) {
        save::ofs.close();
        save::open_ofstream(ofbody, m_force);

        save::note(m_print_date);
        save_data(templates::license);
        save::include_header(header_name, m_pfx_upper);

        substitute(body_data);
    }
}

