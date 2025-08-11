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
    extern const template_t *ptr_license;
    extern const template_t *ptr_filename_macros;
}


/**
 * convert from string to wstring;
 * this is required because on MinGW std::filesystem will throw an exception
 * if an input string contains non-ASCII characters (this doesn't happend with MSVC)
 */
#ifdef __MINGW32__

/* anonymous */
namespace
{
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

    std::wstring string_to_wstring(const std::string &str)
    {
        wchar_t *buf = char_to_wchar(str.c_str());

        if (!buf) {
            std::string msg = __FILE__;
            msg += ": char_to_wchar() failed to convert string to wide characters: ";
            msg += str;

            throw gendlopen::error(msg);
        }

        std::wstring ws = buf;
        delete[] buf;

        return ws;
    }
}
#endif /* __MINGW32__ */


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
 */
void format_libname(const std::string &str, std::string &lib_a, std::string &lib_w, const std::string &pfx)
{
    switch(str.front())
    {
    case 'N':
    case 'n':
        /* no quotes */
        if (utils::prefixed_and_longer_case(str, "nq:")) {
            lib_a = lib_w = str.substr(3);
            return;
        }
        break;

    case 'E':
    case 'e':
        /* quotes + file extension macro */
        if (utils::prefixed_and_longer_case(str, "ext:")) {
            auto sub = str.substr(4);
            lib_a = quote_lib(sub, false) + ' ' + pfx + "_LIBEXTA";
            lib_w = quote_lib(sub, true)  + ' ' + pfx + "_LIBEXTW";
            return;
        }
        break;

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
                return;
            }
        }
        break;

    default:
        break;
    }

    /* quote string */
    lib_a = quote_lib(str, false);
    lib_w = quote_lib(str, true);
}

/* print note */
size_t note(bool print_date)
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

    ofs << strm.str();

    return utils::count_linefeed(strm.str());
}


/* extra defines */
size_t extra_defines(const std::string &defs)
{
    if (defs.empty()) {
        return 0;
    }

    std::string str = "/* extra defines */\n";
    str += defs + '\n';
    ofs << str;

    return utils::count_linefeed(str);
}

/* typedefs for function pointers */
size_t typedefs(const vstring_t &tdefs)
{
    if (tdefs.empty()) {
        return 0;
    }

    std::string str = "/* typedefs */\n";

    for (const auto &e : tdefs) {
        str += "typedef " + e + ";\n";
    }

    str += '\n';
    ofs << str;

    return utils::count_linefeed(str);
}

/* extra includes */
size_t includes(const vstring_t &incs, bool is_cxx)
{
    std::string str = "/* extra headers */\n";

    if (is_cxx) {
        str += "#include <cstddef>\n"  /* size_t, NULL, ... */
               "#include <cstring>\n"; /* strncmp() */
    } else {
        str += "#include <stddef.h>\n"
               "#include <string.h>\n";
    }

    for (auto &e : incs) {
        str += "#include " + e + '\n';
    }

    str += '\n';
    ofs << str;

    return utils::count_linefeed(str);
}

size_t header_guard_begin(const std::string &header_guard, bool is_cxx)
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

    ofs << str;

    return utils::count_linefeed(str);
}

/* don't need to count lines anymore */
void header_guard_end(const std::string &header_guard, bool is_cxx)
{
    if (!is_cxx) {
        /* extern C end */
        ofs << "#ifdef __cplusplus\n"
            "} /* extern \"C\" */\n"
            "#endif\n";
    }

    if (!header_guard.empty()) {
        ofs << "\n#endif //" << header_guard << "_\n";
    }
}

} /* end namespace save */


/* save data, replace prefixes */
size_t gendlopen::save_data(const template_t *list)
{
    size_t total_lines = 0;

    if (!m_line_directive && strncmp(list->data, "#line", 5) == 0) {
        /* skip initial line directive */
        list++;
    }

    for ( ; list->data != NULL; list++) {
        std::string buf;

        if (m_pfx_upper == "GDO") {
            /* nothing to replace */
            buf = list->data;
        } else {
            buf = replace_prefixes(list->data);
        }

        save::ofs << buf << '\n';

        total_lines += utils::count_linefeed(buf);
    }

    return total_lines;
}


/* open output file for writing */
void gendlopen::open_ofstream(const fs::path &opath)
{
    if (opath.empty() || opath == "-") {
        /* default to STDOUT */
        save::ofs.close();
        return;
    }

    /* delete file to prevent writing data into symlink target */
    if (m_force) {
        fs::remove(opath);
    }

    /* check symlink and not its target */
    if (fs::exists(fs::symlink_status(opath))) {
        throw error("file already exists: " + opath.string());
    }

    /* open file for writing */
    if (!save::ofs.open(opath)) {
        throw error("failed to open file for writing: " + opath.string());
    }
}


/* generate output */
void gendlopen::generate()
{
    fs::path ofhdr, ofbody;
    std::string header_name, header_guard;
    vtemplate_t header_data, body_data;
    size_t lines = 0;
    bool is_cxx = false;
    bool gen_macro_header = false;
    bool gen_macro_body = false;

    /************* lambda functions *************/

    auto print_lineno = [&, this] () {
        if (m_line_directive) {
            lines++;
            save::ofs << "#line " << (lines + 1) << " \"" << header_name << "\"\n";
        }
    };

    auto NOTE_AND_LICENSE = [&, this] () {
        lines += save::note(m_print_date);
        lines += save_data(templates::ptr_license);
    };

    auto HEADER_GUARD_BEGIN = [&, this] () {
        if (!m_pragma_once) {
            header_guard = '_' + m_pfx_upper + '_' + utils::to_upper(header_name) + '_';
        }
        lines += save::header_guard_begin(header_guard, is_cxx);
    };

    auto HEADER_FILENAME_MACROS = [&] () {
        lines += save_data(templates::ptr_filename_macros);
    };

    auto HEADER_GENERATED_DATA = [&, this] () {
        /* print extra data after filename macros as includes or defines
         * might make use of it */
        if (m_line_directive) {
            print_lineno();
            save::ofs << '\n'; /* extra padding */
            lines++;
        }

        lines += save::extra_defines(m_defines);
        lines += save::includes(m_includes, is_cxx);
        lines += save::typedefs(m_typedefs);

        if (gen_macro_header) {
            lines += save::symbol_name_lookup(m_pfx_upper, m_prototypes, m_objects);
        }
    };

    auto HEADER_TEMPLATE_DATA = [&] () {
        lines += substitute(header_data);
    };

    auto HEADER_GUARD_END = [&] () {
        print_lineno();
        /* no more line counting needed from here on */
        save::header_guard_end(header_guard, is_cxx);
    };

    auto BODY_GENERATED_DATA = [&, this] () {
        save::ofs << '\n';
        save::ofs << "#define " << m_pfx_upper << "_INCLUDED_IN_BODY\n";
        save::ofs << "#include \"" << header_name << "\"\n";
        save::ofs << '\n';

        if (gen_macro_body) {
            save::symbol_name_lookup(m_pfx_upper, m_prototypes, m_objects);
        }
    };

    auto BODY_TEMPLATE_DATA = [&] () {
        substitute(body_data);
    };

    /********************************************/

    /* output filename */

    const bool use_stdout = (m_output == "-");

    if (!use_stdout)
    {
#ifdef __MINGW32__
        ofbody = ofhdr = string_to_wstring(m_output);
#else
        ofbody = ofhdr = m_output;
#endif
    }

    switch (m_format)
    {
    case output::c:
        if (m_separate) {
            gen_macro_body = true;
        } else {
            gen_macro_header = true;
        }
        break;

    case output::cxx:
        gen_macro_header = true;
        is_cxx = true;
        break;

    case output::minimal:
        m_separate = false;
        break;

    case output::minimal_cxx:
        m_separate = false;
        is_cxx = true;
        break;

    case output::plugin:
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
    }

    /* default library name */
    if (!m_default_lib.empty()) {
        std::string lib_a, lib_w;
        save::format_libname(m_default_lib, lib_a, lib_w, m_pfx_upper);
        m_defines += "#define " + m_pfx_upper + "_HARDCODED_DEFAULT_LIBA " + lib_a + '\n';
        m_defines += "#define " + m_pfx_upper + "_HARDCODED_DEFAULT_LIBW " + lib_w + '\n';
    }

    /* save pointers to template lines in header_data and body_data */
    data::create_template_lists(header_data, body_data, m_format, m_separate);


    /*************** header data ***************/
    open_ofstream(ofhdr);

    NOTE_AND_LICENSE();
    HEADER_GUARD_BEGIN();

    HEADER_FILENAME_MACROS();
    HEADER_GENERATED_DATA();

    HEADER_TEMPLATE_DATA();

    HEADER_GUARD_END();

    save::ofs.close();


    /**************** body data ****************/
    if (!body_data.empty()) {
        open_ofstream(ofbody);

        NOTE_AND_LICENSE();
        BODY_GENERATED_DATA();
        BODY_TEMPLATE_DATA();
    }
}

