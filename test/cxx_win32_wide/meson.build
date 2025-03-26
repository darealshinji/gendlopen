gen_hdr = custom_target('cxx_test.hpp',
    output : 'cxx_test.hpp',
    input : '../helloworld.txt',
    command : [gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@', '-format=C++']
)

cpp_args = ['-DUNICODE', '-D_UNICODE']
link_args = []

if arg_syntax != 'msvc'
    cpp_args += '-municode'
    link_args += '-municode'
endif

e = executable('main', ['main.cpp', gen_hdr],
    dependencies : dl_dep,
    install : false,
    cpp_args : cpp_args,
    link_args : link_args,
    include_directories : include_directories('../00_library')
)

test('C++ win32 wide characters API', e, env : ld_library_path)
