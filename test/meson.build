subdir('00_library')
subdir('00_c')
subdir('00_cxx')

subdir('c_autoload')
subdir('c_auto_release')
subdir('c_clang_ast')
subdir('c_cxx_with_c_header')
subdir('c_cxx_with_c_minimal_header')
subdir('c_line')
subdir('c_load_symbol')
subdir('c_minimal')
if sdl_dep.found()
    subdir('c_minimal_sdl')
endif
subdir('c_multi')
subdir('c_no_date')
subdir('c_option')
subdir('c_param_create')
subdir('c_param_skip')
subdir('c_prefix')
subdir('c_static_library')
subdir('c_static_linkage')
subdir('c_stdin')
if host_machine.system() == 'windows'
    subdir('c_win32_wide')
endif
subdir('c_wrapped_functions')

subdir('cxx_autoload')
subdir('cxx_minimal')
if sdl_dep.found()
    subdir('cxx_minimal_sdl')
endif
subdir('cxx_multi')
subdir('cxx_prefix')
if host_machine.system() == 'windows'
    subdir('cxx_win32_wide')
endif

