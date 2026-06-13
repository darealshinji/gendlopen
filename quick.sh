#!/bin/sh
set -e
set -x

EXTERNAL=false
DEBUG=false


mkdir build

if [ ${EXTERNAL} = true ]; then
    meson setup -Denable_external=true build
    meson compile -C build
    ./build/src/gendlopen tests/helloworld.txt -line -templates-path=src/templates > build/test.h
elif [ ${DEBUG} = true ]; then
    meson setup -Dbuildtype=debug \
        -Denable_tests=true \
        -Denable_examples=true \
        -Dc_args='-fsanitize=undefined' \
        -Dcpp_args='-fsanitize=undefined' \
        -Dc_link_args='-fsanitize=undefined' \
        -Dcpp_link_args='-fsanitize=undefined' \
        build
    meson compile -C build
    meson test -C build
else
    meson setup -Denable_tests=true -Denable_examples=true build
    meson compile -C build
    meson test -C build
fi

