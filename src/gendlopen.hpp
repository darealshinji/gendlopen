/****
  The MIT License (MIT)

  Copyright (C) 2023 djcj@gmx.de

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE
****/

#ifndef _GENDLOPEN_HPP_
#define _GENDLOPEN_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define EXTERN_C_START \
    "#ifdef __cplusplus\n" \
    "extern \"C\" {\n" \
    "#endif\n\n"

#define EXTERN_C_END \
    "#ifdef __cplusplus\n" \
    "} /* extern \"C\" */\n" \
    "#endif\n\n"


class gendlopen
{
public:

    typedef struct {
        std::string type;
        std::string symbol;
        std::string args;
        std::string notype_args;
    } proto_t;

    typedef struct {
        std::string type;
        std::string symbol;
    } obj_t;

private:

    std::vector<proto_t> m_prototypes;
    std::vector<obj_t> m_objects;
    std::string m_typedefs;
    std::string m_name_upper, m_name_lower;
    std::string m_guard;
    bool m_cxx, m_force, m_separate;

    bool tokenize(const std::string &ifile);
    bool tokenize_function(const std::string &s);
    bool tokenize_object(const std::string &s);

    std::string parse(const char *data);

    /* helper to put header guards around the data and save
     * them to the provided stream */
    template<typename T=std::ofstream>
    void put_header_guards(T &str, const char *header_data, const char *body_data, const char *license_data)
    {
        str << license_data;
        str << "#ifndef " << m_guard << '\n';
        str << "#define " << m_guard << "\n\n";

        if (!m_cxx) {
            str << EXTERN_C_START;
        }

        str << parse(header_data);
        str << parse(body_data);

        if (!m_cxx) {
            str << EXTERN_C_END;
        }

        str << "#endif //" << m_guard << '\n';
    }

public:

    /* c'tor */
    gendlopen()
    {
        m_cxx = m_force = m_separate = false;
    }

    /* d'tor */
    virtual ~gendlopen()
    {}

    /* replace_string(a,b,s) will substitute a with b in s */
    inline static void replace_string(const std::string &from, const std::string &to, std::string &s)
    {
        for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
        {
            s.replace(pos, from.size(), to);
        }
    }

    /* set options */
    void separate(bool b) { m_separate = b; }
    void force(bool b) { m_force = b; }
    void cxx(bool b) { m_cxx = b; }

    /* generate output */
    void generate(const std::string &ifile, const std::string &ofile, const std::string &name);

};

#endif //_GENDLOPEN_HPP_

