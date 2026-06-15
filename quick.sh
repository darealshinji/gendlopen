#!/bin/sh
set -e
set -x

DEBUG=false

mkdir build

if [ ${DEBUG} = true ]; then
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

./build/src/gendlopen -templates-path src/templates -format minimal tests/helloworld.txt > build/test_external.h

