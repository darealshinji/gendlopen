#!/bin/sh
set -e

if [ "x$1" != "x" ]; then
    in="$1/"
fi

out="template.h"

hexdump() {
    echo "static const char $2 [] = {" >> $out
    xxd -i < "${in}$1" >> $out
    echo "  , 0x00 };" >> $out
    echo "" >> $out
}

echo "#ifndef _TEMPLATE_H_" > $out
echo "#define _TEMPLATE_H_" >> $out
echo "" >> $out

hexdump "filename_macros.h" "filename_macros_data"
hexdump "license.h"         "license_data"
hexdump "common.h"          "common_header_data"
hexdump "c.h"               "c_header_data"
hexdump "c.c"               "c_body_data"
hexdump "cxx.hpp"           "cxx_header_data"
hexdump "cxx.cpp"           "cxx_body_data"
hexdump "minimal.h"         "min_c_header_data"
hexdump "minimal_cxxeh.hpp" "min_cxx_header_data"

echo "" >> $out
echo "#endif //_TEMPLATE_H_" >> $out

