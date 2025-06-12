#!/bin/sh
set -e
set -x

mkdir build
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
meson test -C build

