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

#include "global.hpp"
#include "template.h"


namespace /* anonymous */
{
    void save_data(cio::ofstream &ofs, const char **data) {
        const char *line = data[0];

        for (int i = 0; line != NULL; i++, line = data[i]) {
            ofs << line << '\n';
        }
        ofs << '\n';
    }
}

namespace data
{
    void save_filename_macros_data(cio::ofstream &ofs) {
        save_data(ofs, filename_macros);
    }

    void save_license_data(cio::ofstream &ofs) {
        save_data(ofs, license);
    }

    /* create template data */
    void concat_templates(cstrList_t &header, cstrList_t &body, output::format format, bool separate)
    {
        switch (format)
        {
        [[unlikely]] default:
            [[fallthrough]];

        case output::c:
            {
                header.push_back(common_header);
                header.push_back(c_header);

                if (separate) {
                    body.push_back(c_body);
                } else {
                    header.push_back(c_body);
                }
            }
            break;

        case output::cxx:
            {
                header.push_back(common_header);
                header.push_back(cxx_header);

                if (separate) {
                    body.push_back(cxx_body);
                } else {
                    header.push_back(cxx_body);
                }
            }
            break;

        case output::minimal:
            header.push_back(min_c_header);
            break;

        case output::minimal_cxx:
            header.push_back(min_cxx_header);
            break;
        }
    }
}
