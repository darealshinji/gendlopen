#!/bin/sh
set -e
set -x

mkdir build
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
meson test -C build

# meson setup -Dwith_external_resources=true build
# ./build/src/gendlopen tests/helloworld.txt -templates-path=src/templates
