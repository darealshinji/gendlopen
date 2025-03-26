#!/bin/sh
set -e
set -x

cd src

flex --noline -o lex.yy.c lex.l
cc -Wall -O2 gen_template_h.c -o gen_template_h
./gen_template_h templates
rm gen_template_h
