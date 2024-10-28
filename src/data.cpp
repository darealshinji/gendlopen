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

/* use a separate compilation unit for the template data */

#include "gendlopen.hpp"
#include "template.h"


/*
namespace
{
    template<size_t N>
    constexpr void save(cstrList_t &trgt, char const (&text)[N])
    {
        trgt.reserve += N;
        trgt.list.push_back(text);

        if (text[N-1] != '\n') {
            trgt.list.push_back("\n");
        }
    }
}
*/

namespace data
{
    const char *filename_macros_data() {
        return filename_macros;
    }

    const char *license_data() {
        return license;
    }

    /* create template data */
    void concat_templates(cstrList_t &header, cstrList_t &body, output::format format, bool separate)
    {
        auto save = [] (cstrList_t &trgt, const char *text, const size_t len) {
            trgt.reserve += len;
            trgt.list.push_back(text);
        };

        header.reserve = 0;
        body.reserve = 0;

        switch (format)
        {
        [[unlikely]] default:
            [[fallthrough]];

        case output::c:
            {
                save(header, common_header, common_header_LENGTH);
                save(header, c_header, c_header_LENGTH);

                if (separate) {
                    save(body, c_body, c_body_LENGTH);
                } else {
                    save(header, c_body, c_body_LENGTH);
                }
            }
            break;

        case output::cxx:
            {
                save(header, common_header, common_header_LENGTH);
                save(header, cxx_header, cxx_header_LENGTH);

                if (separate) {
                    save(body, cxx_body, cxx_body_LENGTH);
                } else {
                    save(header, cxx_body, cxx_body_LENGTH);
                }
            }
            break;

        case output::minimal:
            save(header, min_c_header, min_c_header_LENGTH);
            break;

        case output::minimal_cxx:
            save(header, min_cxx_header, min_cxx_header_LENGTH);
            break;
        }
    }
}
