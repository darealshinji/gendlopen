/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
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

#ifndef GDO_CIO_OFSTREAM_HPP
#define GDO_CIO_OFSTREAM_HPP


namespace cio
{

class ofstream
{
private:

    bool m_stdout;
    std::ofstream m_ofs;

public:

    ofstream();
    virtual ~ofstream();

    /* enable binary mode by default because of Windows */
    bool open(const std::string &file, std::ios::openmode mode = std::ios::out | std::ios::binary);

    bool is_open() const;
    void close();

    /* overloading "<<" operator */
    template<class T>
    std::ostream& operator<<(const T &obj)
    {
        if (m_stdout) {
            return std::cout << obj;
        }
        return m_ofs << obj;
    }
};

} /* namespace cio */

#endif /* GDO_CIO_OFSTREAM_HPP */
