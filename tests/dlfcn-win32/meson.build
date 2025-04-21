
libdl = shared_library('dl',
    'dlfcn.c',
    c_args : '-DDLFCN_WIN32_SHARED',
    install : false
)

hw = '../helloworld.txt'

components = [
    ['',    'c_win32_dlopen',    'C using dlopen() on win32',    hw,  ['-DGDO_USE_DLOPEN']],
    ['pp',  'cxx_win32_dlopen',  'C++ using dlopen() on win32',  hw,  ['-format=C++', '-DGDO_USE_DLOPEN']]
]

foreach p : components
    command = [gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@']

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
        c_args : test_flags,
        cpp_args : test_flags,
        link_with : libdl,
        build_rpath : test_rpath,
        install : false)

    test(p[2], e, env : ld_library_path)
endforeach

