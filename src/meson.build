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
        [gendlopen_src, gen_hdr],
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

