project('gendlopen', ['c', 'cpp'],
    license : 'MIT',
    default_options : {
        'buildtype' : 'release',
        'cpp_std' :
            ['gnu++26', 'c++26',
             'gnu++23', 'c++23',
             'gnu++20', 'c++20',
             'gnu++17', 'c++17',
             'gnu++14', 'c++14',
             'gnu++11', 'c++11']
    }
)

wno_deprecated_declarations = []
cpp_args_extra = []
dl_dep   = []
gtk_dep  = []
glib_dep = []
x11_dep  = []
sdl_dep  = []

compiler          = meson.get_compiler('cpp')
host_system       = host_machine.system()
host_system_win32 = false

if host_system == 'linux' or host_system == 'gnu'
    add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])
elif host_system == 'windows' or host_system == 'cygwin'
    add_project_arguments('-DWIN32_LEAN_AND_MEAN', language: ['c', 'cpp'])
    host_system_win32 = true
endif

if compiler.has_function('program_invocation_short_name', prefix : '#include <errno.h>')
    cpp_args_extra = '-DHAVE_PROGRAM_INVOCATION_SHORT_NAME'
elif compiler.has_function('getprogname', prefix : '#include <stdlib.h>')
    cpp_args_extra = '-DHAVE_GETPROGNAME'
elif compiler.has_header('libgen.h')
    cpp_args_extra = ['-DHAVE_LIBGEN_H']

    if compiler.has_function('basename', prefix : '#include <libgen.h>')
        cpp_args_extra += ['-DHAVE_BASENAME']
    endif
endif

if not host_system_win32
    dl_dep = dependency('dl', required: false)
endif

if not host_system_win32 and get_option('enable_examples')
    if (compiler.has_argument('-Wno-deprecated-declarations'))
        wno_deprecated_declarations = '-Wno-deprecated-declarations'
    endif

    gtk_dep = dependency('gtk+-3.0', required: false).partial_dependency(compile_args: true)

    if not gtk_dep.found()
        gtk_dep = dependency('gtk+-2.0', required: false).partial_dependency(compile_args: true)
    endif

    glib_dep = dependency('glib-2.0', required: false).partial_dependency(compile_args: true)
    x11_dep = dependency('x11', required: false).partial_dependency(compile_args: true)
endif

if get_option('enable_tests') or get_option('enable_examples')
    sdl_dep = dependency('sdl3', required: false)

    if not sdl_dep.found()
        sdl_dep = dependency('sdl2', required: false)
    endif
endif


subdir('src')


if get_option('enable_examples')
    subdir('examples')
endif

if get_option('enable_tests')
    subdir('tests')
endif

