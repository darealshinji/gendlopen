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

#include "cio_ofstream.hpp"


namespace cio
{

ofstream::ofstream()
{}

ofstream::~ofstream()
{
    close();
}

bool ofstream::open(const std::filesystem::path &path)
{
    close();
    m_ofs.open(path, std::ios_base::out | std::ios_base::binary);

    if (m_ofs.is_open()) {
        m_optr = &m_ofs;
        return true;
    }

    return false;
}

bool ofstream::open(const std::string &file)
{
    /* STDOUT */
    if (file == "-") {
        close();
        return true;
    }

    close();
    m_ofs.open(file, std::ios_base::out | std::ios_base::binary);

    if (m_ofs.is_open()) {
        m_optr = &m_ofs;
        return true;
    }

    return false;
}

void ofstream::close()
{
    if (m_ofs.is_open()) {
        m_ofs.close();
    }

    m_optr = &std::cout;
    //std::cout.flush();
}

} /* namespace cio */
