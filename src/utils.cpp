/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2026 Carsten Janssen

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

#include <cctype>
#include <string>
#include "utils.hpp"


/* convert string to uppercase */
std::string utils::to_upper(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (::isalnum(c)) {
                out += static_cast<char>(::toupper(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(::toupper(c));
        }
    }

    return out;
}

/* convert string to lowercase */
std::string utils::to_lower(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (::isalnum(c)) {
                out += static_cast<char>(::tolower(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(::tolower(c));
        }
    }

    return out;
}

