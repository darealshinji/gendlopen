#!/bin/sh
set -e
set -x

cd ..

# https://github.com/mstorsjo/msvc-wine
export WINSYSROOT="$HOME/opt/msvc-wine/msvc"
export CLANGVER=19
export PATH="$PWD/cross-files/clang-cl-wrapper:$PATH"

mkdir build
meson setup -Denable_tests=true -Denable_examples=true --cross-file cross-files/x86_64-windows-msvc-clang.ini build
meson compile -C build
meson test -C build

