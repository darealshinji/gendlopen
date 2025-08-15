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

#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "types.hpp"

#define OUTDIR "templates"


namespace fs = std::filesystem;


namespace templates
{
#include "template.h"
}


namespace /* anonymous */
{
    /* save template data to file */
    void save_to_file(const std::string &filename, const template_t *list)
    {
        cio::ofstream ofs;

        if (!ofs.open(filename)) {
            throw gendlopen::error("failed to open file for writing: " + filename);
        }

        std::cout << "saving " << filename << std::endl;

        for ( ; list->line_count != 0; list++) {
            ofs << list->data << '\n';
        }
    }
}


/* dummy */
void gendlopen::get_templates_path_env()
{
}


/* dummy */
void gendlopen::load_template(templates::name)
{
}


/* dump templates */
void gendlopen::dump_templates()
{
    if (fs::exists(fs::symlink_status(OUTDIR))) {
        throw error("`" OUTDIR "' already exists");
    }

    if (!fs::create_directory(OUTDIR)) {
        throw error("failed to create directory `" OUTDIR "'");
    }

#define SAVE(x) save_to_file(OUTDIR "/" FILENAME_##x, templates::ptr_##x)

    SAVE(license);
    SAVE(filename_macros);
    SAVE(common_header);
    SAVE(c_header);
    SAVE(c_body);
    SAVE(cxx_header);
    SAVE(cxx_body);
    SAVE(plugin_header);
    SAVE(plugin_body);
    SAVE(min_c_header);
    SAVE(min_cxx_header);
}
