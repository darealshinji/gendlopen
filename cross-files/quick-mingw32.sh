#!/bin/sh
set -e
set -x

cd ..

# https://github.com/mxe/mxe
export PATH="$HOME/opt/mxe/usr/bin:$PATH:$HOME/opt/mxe/usr/x86_64-pc-linux-gnu/bin"

mkdir build
meson setup -Denable_tests=true -Denable_examples=true --cross-file cross-files/x86_64-w64-mingw32.static.ini build
meson compile -C build
meson test -C build

