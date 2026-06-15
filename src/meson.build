gendlopen_src = files(
    'cio_ofstream.cpp',
    'check_pattern.cpp',
    'clang_ast.cpp',
    'data.cpp',
    'gendlopen.cpp',
    'generate.cpp',
    'help.cpp',
    'lex.yy.c',
    'main.cpp',
    'open_file.cpp',
    'options.cpp',
    'parameter_names.cpp',
    'parse.cpp',
    'substitute.cpp',
    'tokenize.cpp',
    'utils.cpp'
)

gendlopen_deps = gendlopen_src

# templates_data.cpp
gen_templates_data = executable(
    'gen_templates_data',
    'gen_templates_data.c',
    win_subsystem : 'console',
    install : false,
    native : true
)

gen_sources = custom_target(
    'templates_data.cpp',
    depends : gen_templates_data,
    output : 'templates_data.cpp',
    command : [gen_templates_data, '@CURRENT_SOURCE_DIR@/templates', '@OUTPUT@'])

gendlopen_deps += gen_sources

# manifest
if host_is_win32
    if arg_syntax == 'msvc'
        fs = import('fs')
        fs.copyfile('gendlopen.manifest')
        link_args_extra += ['-manifest:embed', '-manifestinput:src/gendlopen.manifest']
    else
        windows = import('windows')
        gendlopen_deps += windows.compile_resources('gendlopen.rc')
    endif
endif

gendlopen_bin = executable('gendlopen',
    gendlopen_deps,
    cpp_args : cpp_args_extra,
    link_args : link_args_extra,
    win_subsystem : 'console'
)

