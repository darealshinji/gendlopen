project('gendlopen', ['c', 'cpp'],
    license : 'MIT',
    default_options : [
        'b_ndebug=if-release',
        'cpp_std=c++20',
        'buildtype=release'
    ]
)


compiler = meson.get_compiler('c')
arg_syntax = compiler.get_argument_syntax()

link_args = []

if host_machine.system() == 'linux' or host_machine.system() == 'gnu'
    add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp'])
elif host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
    add_project_arguments('-DWIN32_LEAN_AND_MEAN', language: ['c', 'cpp'])
elif host_machine.system() == 'darwin'
    #arg_minver = '-mmacosx-version-min=10.15'  # for std::filesystem::path
    #add_project_arguments(arg_minver, language: ['c', 'cpp'])
    #link_args += arg_minver
endif


# library load path
if host_machine.system() == 'netbsd'
    ld_library_path = 'LD_LIBRARY_PATH=$ORIGIN:/usr/pkg/lib'
elif host_machine.system() == 'haiku'
    ld_library_path = 'LIBRARY_PATH=$ORIGIN'
else
    ld_library_path = 'LD_LIBRARY_PATH=$ORIGIN'
endif


dl_dep = []

if host_machine.system() != 'windows'
    dl_dep = dependency('dl', required: false)
    gtk_dep = dependency('gtk+-2.0', required: false).partial_dependency(compile_args: true)
    glib_dep = dependency('glib-2.0', required: false).partial_dependency(compile_args: true)
    x11_dep = dependency('x11', required: false).partial_dependency(compile_args: true)
endif

sdl_dep = dependency('sdl3', required: false)

if not sdl_dep.found()
    sdl_dep = dependency('sdl2', required: false)
endif


subdir('src')

gendlopen_bin = executable(
    'gendlopen',
    gendlopen_src,
    link_args : link_args,
    win_subsystem : 'console'
)

if get_option('enable_examples')
    subdir('examples')
endif

if get_option('enable_tests')
    subdir('tests')
endif

