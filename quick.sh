#!/bin/sh
set -e
set -x

mkdir build
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
meson test -C build

# meson setup -Denable_external=true build
# meson compile -C build

# ./build/src/gendlopen tests/helloworld.txt -templates-path=src/templates

# set TEMPLATES=src\templates
# build\src\gendlopen.exe tests\helloworld.txt

