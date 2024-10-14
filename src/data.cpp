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


namespace /* anonymous */
{
    template<size_t N>
    constexpr void pb(cstrList_t &trgt, char const (&text)[N])
    {
        trgt.reserve += N;
        trgt.list.push_back(text);

        if (text[N-1] != '\n') {
            trgt.list.push_back("\n");
        }
    }
}

namespace data
{
    const char *filename_macros() {
        return filename_macros_data;
    }

    const char *license() {
        return license_data;
    }

    /* create template data */
    void concat_templates(
        cstrList_t &header_data,
        cstrList_t &body_data,
        output::format format,
        bool separate)
    {
        header_data.reserve = body_data.reserve = 0;

        switch (format)
        {
        [[unlikely]] default:
            [[fallthrough]];

        case output::c:
            {
                pb(header_data, common_header_data);
                pb(header_data, c_header_data);

                if (separate) {
                    pb(body_data, c_body_data);
                } else {
                    pb(header_data, c_body_data);
                }
            }
            break;

        case output::cxx:
            {
                pb(header_data, common_header_data);
                pb(header_data, cxx_header_data);

                if (separate) {
                    pb(body_data, cxx_body_data);
                } else {
                    pb(header_data, cxx_body_data);
                }
            }
            break;

        case output::minimal:
            pb(header_data, min_c_header_data);
            break;

        case output::minimal_cxx:
            pb(header_data, min_cxx_header_data);
            break;
        }
    }
}
