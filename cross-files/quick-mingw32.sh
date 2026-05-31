#!/bin/sh
set -e
set -x

cd ..

mkdir build
meson setup -Denable_tests=true \
    -Denable_examples=true \
    -Denable_utf8=true \
    -Dcpp_link_args='-static-libgcc -static-libstdc++ -s' \
    --cross-file cross-files/x86_64-w64-mingw32.ini \
    build

meson compile -C build
meson test -C build

