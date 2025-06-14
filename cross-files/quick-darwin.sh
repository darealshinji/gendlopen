#!/bin/sh
set -e
set -x

cd ..

# https://github.com/darlinghq/darling
# https://github.com/tpoechtrager/osxcross
export PATH="/opt/darling/bin:$HOME/opt/osxcross-src/target/bin:$PATH"

mkdir build
meson setup -Denable_tests=true -Denable_examples=true --cross-file cross-files/x86_64-apple-darwin24.ini build
meson compile -C build
meson test -C build

