test_flags      = []
ld_library_path = []
test_rpath      = '$ORIGIN'


if arg_syntax == 'msvc'
    test_overrides = {'cpp_std' : 'c++14'}
else
    test_overrides = {'cpp_std' : 'c++11'}
endif

if host_is_win32
    test_flags = ['-DBUILDING_STATIC']  # avoid dllimport/dllexport issues
elif host_system == 'darwin'
    test_rpath = '@executable_path'
endif

# set library path additionally to the rpath as backup
if host_is_win32 == false
    ld_library_path = 'LD_LIBRARY_PATH=$ORIGIN'
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
    ['',    'c_test',                 'C',                                    hw,                             []],
    ['',    'c_autoload',             'C automatic loading',                  hw,                             []],
    ['',    'c_auto_release',         'C automatic release',                  hw,                             []],
    ['',    'c_clang_ast',            'C generated from clang AST',           'ast.txt',                      symbol_list],
    ['',    'c_load_symbol',          'C load individual symbols',            hw,                             []],
    ['',    'c_minimal',              'C minimal header',                     hw,                             ['-format', 'minimal']],
    ['',    'c_line',                 'C with #line directives',              hw,                             ['-line']],
    ['',    'c_no_date',              'C generated without date in header',   hw,                             ['-no-date']],
    ['',    'c_option',               'C generated with %option lines',       'helloworld_option.txt',        []],
    ['',    'c_param_create',         'C parameter names created',            'helloworld_param_create.txt',  ['-param=create']],
    ['',    'c_param_skip',           'C parameter names skipped',            hw,                             ['-param=skip']],
    ['',    'c_prefix',               'C custom symbol prefix',               hw,                             ['-prefix', 'MyPrefix']],
    ['',    'c_static_linkage',       'C static inline linkage',              hw,                             []],
    ['',    'c_wrapped_functions',    'C wrapped functions',                  hw,                             []],
    ['pp',  'cxx_test',               'C++',                                  hw,                             ['-format=c++']],
    ['pp',  'cxx_autoload',           'C++ automatic loading',                hw,                             ['-format=c++']],
    ['pp',  'cxx_minimal',            'C++ minimal header',                   hw,                             ['-format', 'minimal-c++']],
    ['pp',  'cxx_prefix',             'C++ custom symbol prefix',             hw,                             ['-format=C++', '-prefix=MyPrefix']],
    ['pp',  'cxx_wrapped_functions',  'C++ wrapped functions',                hw,                             ['-format=c++']]
]

foreach p : components
    command = [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']

    foreach arg : p[4]
        command += arg
    endforeach

    gen_hdr = custom_target(p[1]+'.h'+p[0],
        depends : helloworld_lib,
        output : p[1]+'.h'+p[0],
        input : p[3],
        command : command)

    e = executable(p[1], [p[1]+'.c'+p[0], gen_hdr],
        dependencies : dl_dep,
        override_options : test_overrides,
        c_args : test_flags,
        cpp_args : test_flags,
        build_rpath : test_rpath,
        install : false)

    test(p[2], e, env : ld_library_path)
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

components = [
    ['c_',   '',   'C'],
    ['cxx_', 'pp', 'C++']
]

foreach p : components
    gen_srcs = custom_target(p[0]+'multi_generated.c'+p[1],
        depends : helloworld_lib,
        output : [p[0]+'multi_generated.c'+p[1], p[0]+'multi_generated.h'+p[1]],
        input : hw,
        command : [
            gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT1@', '-format', p[2],
            '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
            '-DGDO_ENABLE_AUTOLOAD'
        ]
    )

    e = executable(p[0]+'multi', [p[0]+'multi.c'+p[1], p[0]+'multi_cb.c'+p[1], p[0]+'multi_call2.c'+p[1], gen_srcs],
        dependencies : dl_dep,
        override_options : test_overrides,
        c_args : test_flags,
        cpp_args : test_flags,
        build_rpath : test_rpath,
        install : false
    )

    test(p[2]+' multiple source files', e, env : ld_library_path)
endforeach

foreach p : components
    gen_srcs = custom_target(p[0]+'va_args_inline_generated.c'+p[1],
        depends : helloworld_lib,
        output : [p[0]+'va_args_inline_generated.c'+p[1], p[0]+'va_args_inline_generated.h'+p[1]],
        input : hw,
        command : [
            gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT1@', '-format', p[2],
            '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
            '-DGDO_ENABLE_AUTOLOAD'
        ]
    )

    e = executable(p[0]+'va_args_inline', [p[0]+'va_args_inline.c'+p[1], gen_srcs],
        dependencies : dl_dep,
        override_options : test_overrides,
        c_args : test_flags,
        cpp_args : test_flags,
        build_rpath : test_rpath,
        install : false
    )

    test(p[2]+' VA_ARGS inline, multiple source files', e, env : ld_library_path)
endforeach



### C static library test ###

gen_srcs = custom_target('libhelloworld_generated.c',
    depends : helloworld_lib,
    output : ['libhelloworld_generated.c', 'libhelloworld_generated.h'],
    input : hw,
    command : [
        gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT0@',
        '-separate', '-library=API:0:helloworld', '-include=helloworld.h',
        '-DGDO_ENABLE_AUTOLOAD',
        '-DGDO_WRAP_IS_VISIBLE',
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
    cpp_args = ['-DUNICODE', '-D_UNICODE']
    link_args = []

    if arg_syntax != 'msvc'
        c_args += '-municode'
        cpp_args += '-municode'
        link_args += '-municode'
    endif


    components = [
        ['c_',   '',   'C'],
        ['cxx_', 'pp', 'C++']
    ]

    foreach p : components
        gen_hdr = custom_target(p[0]+'win32_wide.h'+p[1],
            output : p[0]+'win32_wide.h'+p[1],
            input : hw,
            command : [gendlopen_bin, '@INPUT@', '-format', p[2], '-force', '-out', '@OUTPUT@']
        )

        e = executable(p[0]+'win32_wide', [p[0]+'win32_wide.c'+p[1], gen_hdr],
            override_options : test_overrides,
            c_args : [c_args, test_flags],
            cpp_args : [cpp_args, test_flags],
            link_args : link_args,
            install : false
        )

        test(p[2]+' win32 wide characters API', e, env : ld_library_path)
    endforeach
endif



### plugin API ###
subdir('plugins')


### win32 dlopen() API ###
if host_is_win32
    subdir('dlfcn-win32')

    incdir = include_directories('dlfcn-win32')

    components = [
        ['',    'c_win32_dlopen',    'C using dlopen() on win32',    hw,  ['-DGDO_USE_DLOPEN']],
        ['pp',  'cxx_win32_dlopen',  'C++ using dlopen() on win32',  hw,  ['-format=C++', '-DGDO_USE_DLOPEN']]
    ]

    foreach p : components
        command = [gendlopen_bin, '@INPUT@', '-force', '-out', '@OUTPUT@']

        foreach arg : p[4]
            command += arg
        endforeach

        gen_hdr = custom_target(p[1]+'.h'+p[0],
            depends : helloworld_lib,
            output : p[1]+'.h'+p[0],
            input : p[3],
            command : command)

        e = executable(p[1], [p[1]+'.c'+p[0], gen_hdr],
            override_options : test_overrides,
            include_directories : incdir,
            c_args : test_flags,
            cpp_args : test_flags,
            link_with : libdl,
            build_rpath : test_rpath,
            install : false)

        test(p[2], e, env : ld_library_path)
    endforeach
endif

