project('gendlopen', ['c', 'cpp'],
    license : 'MIT',
    default_options : {
        'buildtype' : 'release',
        'cpp_std' :
            ['gnu++26', 'c++26',
             'gnu++23', 'c++23',
             'gnu++20', 'c++20']
    }
)

wno_deprecated_declarations = []
cpp_args_extra = []
dl_dep   = []
gtk_dep  = []
glib_dep = []
x11_dep  = []
sdl_dep  = []


# options

compiler       = meson.get_compiler('cpp')
host_system    = host_machine.system()
host_is_win32  = (host_system == 'windows' or host_system == 'cygwin')
host_is_gnu    = (host_system == 'linux' or host_system == 'gnu')
external_files = get_option('enable_external')
templates_path = get_option('templates_path')

if host_is_gnu and not get_option('without_gnu_source')
    add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])
elif host_is_win32
    add_project_arguments('-DWIN32_LEAN_AND_MEAN', language: ['c', 'cpp'])
endif

if compiler.has_function('program_invocation_short_name', prefix : '#include <errno.h>')
    cpp_args_extra += '-DHAVE_PROGRAM_INVOCATION_SHORT_NAME'
elif compiler.has_function('getprogname', prefix : '#include <stdlib.h>')
    cpp_args_extra += '-DHAVE_GETPROGNAME'
endif

if external_files
    cpp_args_extra += '-DUSE_EXTERNAL_RESOURCES'
endif

if templates_path != ''
    cpp_args_extra += '-DDEFAULT_TEMPLATES_PATH="' + templates_path + '"'
endif




# dependencies

if not host_is_win32
    dl_dep = dependency('dl', required: false)
endif

if not host_is_win32 and get_option('enable_examples')
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


# sub-directories

subdir('src')

if get_option('enable_examples')
    subdir('examples')
endif

if get_option('enable_tests')
    subdir('tests')
endif

