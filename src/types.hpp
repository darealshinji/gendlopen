/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

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

#pragma once

#include <filesystem>
#include <list>
#include <string>
#include <vector>


/* typedefs */

namespace proto
{
    typedef enum {
        function,
        function_pointer,
        object,
        object_array
    } type;
}

using vstring_t = std::vector<std::string>;
using iter_t = vstring_t::iterator;
using list_t = std::list<const char *>;

typedef struct _proto {
    proto::type prototype;
    std::string type;
    std::string symbol;
    std::string args;
    std::vector<vstring_t> args_vec;
    std::string notype_args;
} proto_t;

typedef struct _decl {
    proto::type prototype;
    std::string symbol;
    std::string type;
} decl_t;

typedef struct _template {
    const char *data;
    int maybe_keyword;
    int line_count;
} template_t;

using vproto_t = std::vector<proto_t>;
using vtemplate_t = std::vector<const template_t *>;

#ifdef __cpp_lib_filesystem
using fs_path_t = std::filesystem::path;
#else
using fs_path_t = std::string;
#endif


/* enum for outout format */
namespace output
{
    typedef enum {
        error,
        c,
        cxx,
        plugin,
        minimal,
        minimal_cxx
    } format;
}


/* enum for parameter names */
namespace param
{
    typedef enum {
        read, /* default */
        create,
        skip
    } names;
}

