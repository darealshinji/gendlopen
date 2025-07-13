# About

Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries. It takes text files with C prototype
declarations as input and creates C or C++ header files as output.
It's designed to add dynamic loading support to existing code with minimal effort.


# Motivation

Writing macros and formatted prototype lists to be used with `dlopen()` and
`dlsym()` is annoying, so I wanted to automate this process as much as possible.
I could have written a script but I wanted the tool to be able to be used in
Windows too without the need to install a script interpreter first.
I also didn't want to just write a header-only library relying on X-macros, that
would still require formatting lists by hand and X-macros are complicated and ugly.
So in the end I wrote my own small macro tool.
