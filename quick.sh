#!/bin/sh
set -e
set -x

EXTERNAL=false

mkdir build

if [ ${EXTERNAL} = true ]; then
    meson setup -Denable_external=true build
    meson compile -C build
    ./build/src/gendlopen tests/helloworld.txt -line -templates-path=src/templates > build/test.h
else
    meson setup -Denable_tests=true -Denable_examples=true build
    meson compile -C build
    meson test -C build
fi

