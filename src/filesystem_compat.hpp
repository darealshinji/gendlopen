/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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
#include <string>

#if defined(__cpp_lib_filesystem) && defined(__MINGW32__)
# define MINGW32_NEED_CONVERT_FILENAME
#endif

#if !defined(__cpp_lib_filesystem)
# ifdef _WIN32
#  include <io.h>
#  include <stdio.h>
# else
#  include <unistd.h>
# endif
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include "types.hpp"



namespace fs
{
#ifdef __cpp_lib_filesystem

    /* use features from std::filesystem */

    inline std::string filename(const fs_path_t &path) {
        return path.filename().string();
    }

    inline void replace_extension(fs_path_t &path, const std::string &ext) {
        path.replace_extension(ext);
    }

    inline void remove_file(const fs_path_t &path) {
        std::filesystem::remove(path);
    }

    inline bool exists_lstat(const fs_path_t &path) {
        return std::filesystem::exists(std::filesystem::symlink_status(path));
    }

# ifdef MINGW32_NEED_CONVERT_FILENAME
    std::wstring convert_filename(const std::string &str);
# endif

#else

    /* fall back to using our own implementations */

    std::string filename(const std::string &path);

    void replace_extension(std::string &path, std::string ext);

#ifdef _WIN32

    inline void remove_file(const std::string &path) {
        _unlink(path.c_str());
    }

    inline bool exists_lstat(const std::string &path)
    {
        /* no lstat/_lstat available */
        struct _stat st;
        return (_stat(path.c_str(), &st) == 0);
    }

#else

    inline void remove_file(const std::string &path) {
        unlink(path.c_str());
    }
    
    inline bool exists_lstat(const std::string &path) {
        struct stat st;
        return (lstat(path.c_str(), &st) == 0);
    }

#endif /* _WIN32 */

#endif /* __cpp_lib_filesystem */

} /* end namespace fs */

