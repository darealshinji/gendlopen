gendlopen_src = files(
    'cio_ofstream.cpp',
    'clang-ast.cpp',
    'data.cpp',
    'gendlopen.cpp',
    'generate.cpp',
    'help.cpp',
    'lex.yy.c',
    'main.cpp',
    'open_file.cpp',
    'parameters.cpp',
    'parse_args.cpp',
    'parse.cpp',
    'parse_options.cpp',
    'substitute.cpp',
    'tokenize.cpp',
    'utils.cpp'
)

gendlopen_bin = executable(
    'gendlopen',
    gendlopen_src,
    win_subsystem : 'console'
)


# set up gendlopen as auxilliary tool
if cross_compile and (get_option('enable_examples') or get_option('enable_tests'))
    # disable optimization to speed up build, manually set `-std' value
    if arg_syntax_native == 'msvc'
        c_args   = '-Od'
        cpp_args = ['-Od', '-std:c++latest']
    else
        c_args   = '-O0'
        cpp_args = ['-O0', '-std=c++20']
    endif

    gendlopen_aux_bin = executable(
        'gendlopen_aux',
        gendlopen_src,
        c_args : c_args,
        cpp_args : cpp_args,
        win_subsystem : 'console',
        install : false,
        native : true
    )
else
    # use host tool
    gendlopen_aux_bin = gendlopen_bin
endif

