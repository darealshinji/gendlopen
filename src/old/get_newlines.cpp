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

#include <iterator>
#include <string>


/* return only the leading newlines from string */
std::string get_leading_newlines(const std::string &in)
{
    size_t i = 0;

    while (i < in.size()) {
        if (in.at(i) == '\n') {
            /* Unix newline */
            i++;
        } else if (in.compare(i, 2, "\r\n") == 0) {
            /* Windows newline */
            i += 2;
        } else {
            break;
        }
    }

    return in.substr(0, i);
}

/* return only the trailing newlines from string */
const char *get_trailing_newlines(const std::string &in)
{
    auto it = in.rbegin();

    while (it != in.rend()) {
        if (*it != '\n') {
            break;
        }

        auto next = it + 1;

        if (next != in.rend() && *next == '\r') {
            /* Windows newline */
            it += 2;
        } else {
            /* Unix newline */
            it++;
        }
    }

    return in.c_str() + (in.size() - std::distance(in.rbegin(), it));
}
