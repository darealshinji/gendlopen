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
    'parse.cpp',
    'parse_args.cpp',
    'parse_options.cpp',
    'parse_parameters.cpp',
    'substitute.cpp',
    'symbol_name_lookup.cpp',
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
    cpp_args : cpp_args_extra,
    win_subsystem : 'console'
)

gendlopen_aux_bin = gendlopen_bin

