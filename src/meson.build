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


gen_template_h = executable(
    'gen_template_h',
    'gen_template_h.c',
    win_subsystem : 'console',
    install : false,
    native : true
)

gen_hdr = custom_target(
    'template.h',
    depends : gen_template_h,
    output : 'template.h',
    command : [gen_template_h, '@CURRENT_SOURCE_DIR@/templates', '@OUTPUT@'])

gendlopen_bin = executable(
    'gendlopen',
    [gendlopen_src, gen_hdr],
    win_subsystem : 'console'
)


# set up gendlopen as auxilliary tool
if cross_compile and (get_option('enable_examples') or get_option('enable_tests'))
    noopt = []
    cpp_std = []

    # disable optimization to speed up build,
    # enable C++20 or newer

    if arg_syntax_native == 'msvc'
        noopt = '-Od'
        cpp_std = '-std:c++latest'
    else
        if arg_syntax_native == 'gcc'
            noopt = '-O0'
        endif

        cpp_std = [
            '-std=gnu++26', '-std=c++26',
            '-std=gnu++23', '-std=c++23',
            '-std=gnu++20', '-std=c++20',
        ]

        compiler = meson.get_compiler('cpp', native : true)

        foreach arg : cpp_std
            if (compiler.has_argument(arg))
                cpp_std = arg
                break
            endif
        endforeach
    endif

    gendlopen_aux_bin = executable(
        'gendlopen_aux',
        [gendlopen_src, gen_hdr],
        c_args : noopt,
        cpp_args : [noopt, cpp_std],
        win_subsystem : 'console',
        install : false,
        native : true
    )
else
    # use host tool
    gendlopen_aux_bin = gendlopen_bin
endif

