gen_hdr = custom_target('cxx_test.hpp',
    output : 'cxx_test.hpp',
    input : '../helloworld.txt',
    command : [gendlopen_bin, '@INPUT@', '-force', '-o', '@OUTPUT@', '-format=minimal-c++']
)

e = executable('main', ['main.cpp', gen_hdr],
    dependencies : dl_dep,
    install : false,
    include_directories : include_directories('../00_library')
)

test('C++ minimal header', e, env : ld_library_path)
