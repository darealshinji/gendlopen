project('gendlopen', ['c', 'cpp'],
    license : 'MIT',
    default_options : [
        'cpp_std=c++latest,gnu++26,c++26,gnu++23,c++23,gnu++20,c++20,gnu++17,c++17,gnu++14,c++14,gnu++11,c++11',
        'buildtype=release'
    ]
)


compiler          = meson.get_compiler('cpp')
arg_syntax        = compiler.get_argument_syntax()
arg_syntax_native = meson.get_compiler('cpp', native : true).get_argument_syntax()
host_system       = host_machine.system()
cross_compile     = false
host_system_win32 = false

if build_machine.system() != host_system
    cross_compile = true
endif

if host_system == 'linux' or host_system == 'gnu'
    # needed for tests and examples, but setting it globally doesn't hurt
    add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])
elif host_system == 'windows' or host_system == 'cygwin'
    add_project_arguments('-DWIN32_LEAN_AND_MEAN', language: ['c', 'cpp'])
    host_system_win32 = true
endif

wno_deprecated_declarations = []
dl_dep = []
gtk_dep = []
glib_dep = []
x11_dep = []
sdl_dep = []

if host_system_win32 == false
    dl_dep = dependency('dl', required: false)

    if get_option('enable_examples')
        if (compiler.has_argument('-Wno-deprecated-declarations'))
            wno_deprecated_declarations = '-Wno-deprecated-declarations'
        endif

        gtk_dep = dependency('gtk+-2.0', required: false).partial_dependency(compile_args: true)
        glib_dep = dependency('glib-2.0', required: false).partial_dependency(compile_args: true)
        x11_dep = dependency('x11', required: false).partial_dependency(compile_args: true)
    endif
endif

if get_option('enable_tests') or get_option('enable_examples')
    sdl_dep = dependency('sdl3', required: false)

    if not sdl_dep.found()
        sdl_dep = dependency('sdl2', required: false)
    endif
endif


subdir('src')


# try to set standard to C++11

cpp_std = []

if get_option('enable_tests') or get_option('enable_examples')
    compiler = meson.get_compiler('cpp')

    if (compiler.has_argument('-std=c++11'))
        cpp_std = '-std=c++11'
    elif (compiler.has_argument('-std:c++11'))
        cpp_std = '-std:c++11'
    endif
endif

if get_option('enable_examples')
    subdir('examples')
endif

if get_option('enable_tests')
    subdir('tests')
endif

