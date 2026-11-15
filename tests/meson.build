test_flags      = []
ld_library_path = []
test_rpath      = '$ORIGIN'


if arg_syntax == 'msvc'
    test_overrides = {'cpp_std' : 'c++14'}
else
    test_overrides = {'cpp_std' : 'c++11'}
endif

if host_system == 'darwin'
    test_rpath = '@executable_path'
endif

if host_is_win32
    test_flags += '-DBUILDING_STATIC'  # avoid dllimport/dllexport issues
else
    ld_library_path = 'LD_LIBRARY_PATH=$ORIGIN'  # set additionally to rpath
endif



helloworld_lib = shared_library('helloworld',
    'helloworld.c',
    c_args : '-DBUILDING_DLL',
    soversion : '0',
    install : false
)



### tests ###

hw = 'helloworld.txt'

symbol_list = [
    '-Phelloworld_hello',
    '-Shelloworld_init_argv',
    '-Shelloworld_callback',
    '-Shelloworld_buffer',
    '-Shelloworld_release'
]

components = [
    ['c_test',                 'C',                                    hw,                             []],
    ['c_autoload',             'C automatic loading',                  hw,                             []],
    ['c_auto_release',         'C automatic release',                  hw,                             []],
    ['c_clang_ast',            'C generated from clang AST',           'ast.txt',                      symbol_list],
    ['c_load_symbol',          'C load individual symbols',            hw,                             []],
    ['c_minimal',              'C minimal header',                     hw,                             ['-format', 'minimal']],
    ['c_line',                 'C with #line directives',              hw,                             ['-line']],
    ['c_no_date',              'C generated without date in header',   hw,                             ['-no-date']],
    ['c_option',               'C generated with %option lines',       'helloworld_option.txt',        []],
    ['c_param_create',         'C parameter names created',            'helloworld_param_create.txt',  ['-param=create']],
    ['c_param_skip',           'C parameter names skipped',            hw,                             ['-param=skip']],
    ['c_prefix',               'C custom symbol prefix',               hw,                             ['-prefix', 'MyPrefix']],
    ['c_static_linkage',       'C static inline linkage',              hw,                             []],
    ['c_wrapped_functions',    'C wrapped functions',                  hw,                             []],
]

foreach p : components
    command = [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']

    foreach arg : p[3]
        command += arg
    endforeach

    gen_hdr = custom_target(p[0]+'.h',
        depends : helloworld_lib,
        output : p[0]+'.h',
        input : p[2],
        command : command)

    e = executable(p[0], [p[0]+'.c', gen_hdr],
        dependencies : dl_dep,
        override_options : test_overrides,
        c_args : test_flags,
        cpp_args : test_flags,
        build_rpath : test_rpath,
        install : false)

    test(p[1], e, env : ld_library_path)
endforeach



### C++ with C header tests ###

components = [
    ['cxx_with_c_header',         'C++ source file with C header',         '-format=C'],
    ['cxx_with_c_minimal_header', 'C++ source file with minimal C header', '-format=minimal']
]

foreach p : components
    gen_hdr = custom_target(p[0]+'.h',
        depends : helloworld_lib,
        output : p[0]+'.h',
        input : hw,
        command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@', p[2]])

    e = executable(p[0], [p[0]+'.cpp', gen_hdr],
        dependencies : dl_dep,
        override_options : test_overrides,
        cpp_args : test_flags,
        build_rpath : test_rpath,
        install : false)

    test(p[1], e, env : ld_library_path)
endforeach



### multiple source files tests ###

gen_srcs = custom_target('c_multi_generated.c',
    depends : helloworld_lib,
    output : ['c_multi_generated.c', 'c_multi_generated.h'],
    input : hw,
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT1@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DGDO_ENABLE_AUTOLOAD'
    ]
)

e = executable('c_multi', ['c_multi.c', 'c_multi_cb.c', 'c_multi_call2.c', gen_srcs],
    dependencies : dl_dep,
    override_options : test_overrides,
    c_args : test_flags,
    cpp_args : test_flags,
    build_rpath : test_rpath,
    install : false
)

test('C multiple source files', e, env : ld_library_path)


gen_srcs = custom_target('c_va_args_inline_generated.c',
    depends : helloworld_lib,
    output : ['c_va_args_inline_generated.c', 'c_va_args_inline_generated.h'],
    input : hw,
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT1@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DGDO_ENABLE_AUTOLOAD'
    ]
)

e = executable('c_va_args_inline', ['c_va_args_inline.c', gen_srcs],
    dependencies : dl_dep,
    override_options : test_overrides,
    c_args : test_flags,
    cpp_args : test_flags,
    build_rpath : test_rpath,
    install : false
)

test('C VA_ARGS inline, multiple source files', e, env : ld_library_path)



### C static library test ###

gen_srcs = custom_target('libhelloworld_generated.c',
    depends : helloworld_lib,
    output : ['libhelloworld_generated.c', 'libhelloworld_generated.h'],
    input : hw,
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT0@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DGDO_ENABLE_AUTOLOAD',
        '-DGDO_WRAP_VISIBILITY=',
        '-DGDO_DISABLE_WARNINGS' # helloworld_fprintf isn't used
    ]
)

helloworld_fakelib = static_library('helloworld_dl', gen_srcs,
    c_args : test_flags,
    install : false
)

e = executable('c_static_library', 'c_static_library.c',
    dependencies : dl_dep,
    link_with : helloworld_fakelib,
    c_args : test_flags,
    build_rpath : test_rpath,
    install : false
)

test('C fake static library', e, env : ld_library_path)



### read input from STDIN ###

read_from_stdin = executable('read_from_stdin',
    'read_from_stdin.c',
    install : false
)

gen_hdr = custom_target('c_stdin.h',
    depends : helloworld_lib,
    output : 'c_stdin.h',
    input : [hw, gendlopen_bin],
    command : [read_from_stdin, '@INPUT1@', '@OUTPUT@', '@INPUT0@']
)

e = executable('c_stdin', ['c_stdin.c', gen_hdr],
    dependencies : dl_dep,
    c_args : test_flags,
    build_rpath : test_rpath,
    install : false
)

test('C generated from STDIN', e, env : ld_library_path)



### win32 wide characters API ###

if host_is_win32
    c_args = ['-DUNICODE', '-D_UNICODE']
    link_args = []

    if arg_syntax != 'msvc'
        c_args += '-municode'
        link_args += '-municode'
    endif

    gen_hdr = custom_target('c_win32_wide.h',
        output : 'c_win32_wide.h',
        input : hw,
        command : [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']
    )

    e = executable('c_win32_wide', ['c_win32_wide.c', gen_hdr],
        override_options : test_overrides,
        c_args : [c_args, test_flags],
        link_args : link_args,
        install : false
    )

    test('C win32 wide characters API', e, env : ld_library_path)
endif



### plugin API ###
subdir('plugins')


### win32 dlopen() API ###
if host_is_win32
    subdir('dlfcn-win32')

    incdir = include_directories('dlfcn-win32')

    gen_hdr = custom_target('c_win32_dlopen.h',
        depends : helloworld_lib,
        output : 'c_win32_dlopen.h',
        input : hw,
        command : [gendlopen_bin, '@INPUT@', '-DGDO_USE_DLOPEN', '-force', '-out', '@OUTPUT@']
    )

    e = executable('c_win32_dlopen', ['c_win32_dlopen.c', gen_hdr],
        override_options : test_overrides,
        include_directories : incdir,
        c_args : test_flags,
        cpp_args : test_flags,
        link_with : libdl,
        build_rpath : test_rpath,
        install : false
    )

    test('C using dlopen() on win32', e, env : ld_library_path)
endif

